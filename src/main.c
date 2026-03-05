#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "streampipe/alert.h"
#include "streampipe/filter.h"
#include "streampipe/input.h"
#include "streampipe/queue.h"
#include "streampipe/wal.h"

static volatile sig_atomic_t g_stop_requested = 0;

typedef struct {
    sp_config_t cfg;
    sp_input_t input;
    sp_filter_t filter;
    sp_sample_queue_t sample_queue;
    sp_alert_queue_t alert_queue;
    sp_wal_t wal;
    sp_alert_sender_t sender;
    pthread_t sample_thread;
    pthread_t process_thread;
    pthread_t alert_thread;
} sp_app_t;

void sp_handle_signal(int sig) {
    (void) sig;
    g_stop_requested = 1;
}

static void sp_print_help(const char* prog) {
    printf("StreamPipe - edge sensor pipeline\n");
    printf("Usage: %s [options]\n\n", prog);
    printf("  --mode [sim|uart|spi|i2c]\n");
    printf("  --device /path/to/source\n");
    printf("  --sensor-id N\n");
    printf("  --window N\n");
    printf("  --threshold N\n");
    printf("  --interval-ms N\n");
    printf("  --wal-path /path/to/wal\n");
    printf("  --alert-host ip\n");
    printf("  --alert-port N\n");
    printf("  --help\n");
}

static sp_source_type_t sp_parse_source(const char* text) {
    if (strcmp(text, "uart") == 0) {
        return SP_SOURCE_UART;
    }
    if (strcmp(text, "spi") == 0) {
        return SP_SOURCE_SPI;
    }
    if (strcmp(text, "i2c") == 0) {
        return SP_SOURCE_I2C;
    }
    return SP_SOURCE_SIM;
}

static int sp_parse_args(int argc, char** argv, sp_config_t* cfg) {
    cfg->source = SP_SOURCE_SIM;
    cfg->source_device = "";
    cfg->default_sensor_id = 1;
    cfg->window_size = 24;
    cfg->anomaly_threshold_fp = (int32_t) (3.0f * SP_FP_SCALE);
    cfg->sample_interval_ms = 200;
    cfg->wal_path = "streampipe.wal";
    cfg->alert_host = "127.0.0.1";
    cfg->alert_port = 9999;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
            cfg->source = sp_parse_source(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "--device") == 0 && i + 1 < argc) {
            cfg->source_device = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "--sensor-id") == 0 && i + 1 < argc) {
            cfg->default_sensor_id = (uint32_t) atoi(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "--window") == 0 && i + 1 < argc) {
            cfg->window_size = (uint16_t) atoi(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "--threshold") == 0 && i + 1 < argc) {
            cfg->anomaly_threshold_fp = (int32_t) (atof(argv[++i]) * SP_FP_SCALE);
            continue;
        }
        if (strcmp(argv[i], "--interval-ms") == 0 && i + 1 < argc) {
            cfg->sample_interval_ms = (uint32_t) atoi(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "--wal-path") == 0 && i + 1 < argc) {
            cfg->wal_path = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "--alert-host") == 0 && i + 1 < argc) {
            cfg->alert_host = argv[++i];
            continue;
        }
        if (strcmp(argv[i], "--alert-port") == 0 && i + 1 < argc) {
            cfg->alert_port = (uint16_t) atoi(argv[++i]);
            continue;
        }
        if (strcmp(argv[i], "--help") == 0) {
            sp_print_help(argv[0]);
            return -1;
        }

        fprintf(stderr, "unknown option %s\n", argv[i]);
        return -1;
    }

    if (cfg->window_size == 0 || cfg->window_size > SP_MAX_WINDOW) {
        fprintf(stderr, "invalid window %u, must be 1..%d\n", cfg->window_size, SP_MAX_WINDOW);
        return -1;
    }

    if (cfg->source != SP_SOURCE_SIM && cfg->source_device[0] == '\0') {
        fprintf(stderr, "source device required for mode uart spi i2c\n");
        return -1;
    }

    return 0;
}

static void* sp_sample_thread(void* data) {
    sp_app_t* app = data;
    sp_sample_t sample;

    while (!g_stop_requested) {
        if (sp_input_read(&app->input, &sample) == 0) {
            if (!sp_sample_queue_push(&app->sample_queue, &sample)) {
                break;
            }
            if (app->cfg.source == SP_SOURCE_SIM && app->cfg.sample_interval_ms > 0) {
                usleep(app->cfg.sample_interval_ms * 1000u);
            }
            continue;
        }

        usleep(1000);
    }

    sp_sample_queue_shutdown(&app->sample_queue);
    return NULL;
}

static void* sp_process_thread(void* data) {
    sp_app_t* app = data;
    sp_sample_t sample;
    sp_alert_t alert;

    while (sp_sample_queue_pop(&app->sample_queue, &sample)) {
        if (sp_filter_process(&app->filter, &sample, &alert)) {
            if (!sp_alert_queue_push(&app->alert_queue, &alert)) {
                break;
            }
            sp_wal_append_alert(&app->wal, &alert);
        }
    }

    sp_alert_queue_shutdown(&app->alert_queue);
    return NULL;
}

static void* sp_alert_thread(void* data) {
    sp_app_t* app = data;
    sp_alert_t alert;

    while (sp_alert_queue_pop(&app->alert_queue, &alert)) {
        int rc = sp_alert_sender_send(&app->sender, &alert);
        if (rc != 0) {
            fprintf(stderr, "alert send failed\n");
        }
    }

    return NULL;
}

int main(int argc, char** argv) {
    signal(SIGINT, sp_handle_signal);

    sp_app_t app;
    memset(&app, 0, sizeof(app));

    if (sp_parse_args(argc, argv, &app.cfg) != 0) {
        sp_print_help(argv[0]);
        return 1;
    }

    if (sp_input_init(&app.input, &app.cfg) != 0) {
        fprintf(stderr, "input init failed for source %s\n", app.cfg.source_device);
        return 1;
    }

    if (sp_wal_open(&app.wal, app.cfg.wal_path) != 0) {
        fprintf(stderr, "wal open failed %s\n", app.cfg.wal_path);
        return 1;
    }

    if (sp_alert_sender_init(&app.sender, app.cfg.alert_host, app.cfg.alert_port) != 0) {
        fprintf(stderr, "warning alert sender disabled %s\n", app.cfg.alert_host);
    }

    sp_sample_queue_init(&app.sample_queue, &g_stop_requested);
    sp_alert_queue_init(&app.alert_queue, &g_stop_requested);
    sp_filter_init(&app.filter, &app.cfg);

    if (pthread_create(&app.sample_thread, NULL, sp_sample_thread, &app) != 0) {
        fprintf(stderr, "sample thread create failed\n");
        sp_input_close(&app.input);
        return 1;
    }

    if (pthread_create(&app.process_thread, NULL, sp_process_thread, &app) != 0) {
        fprintf(stderr, "process thread create failed\n");
        g_stop_requested = 1;
        sp_sample_queue_shutdown(&app.sample_queue);
        pthread_join(app.sample_thread, NULL);
        sp_input_close(&app.input);
        return 1;
    }

    if (pthread_create(&app.alert_thread, NULL, sp_alert_thread, &app) != 0) {
        fprintf(stderr, "alert thread create failed\n");
        g_stop_requested = 1;
        sp_sample_queue_shutdown(&app.sample_queue);
        sp_alert_queue_shutdown(&app.alert_queue);
        pthread_join(app.sample_thread, NULL);
        pthread_join(app.process_thread, NULL);
        sp_input_close(&app.input);
        return 1;
    }

    while (!g_stop_requested) {
        sleep(1);
    }

    sp_sample_queue_shutdown(&app.sample_queue);
    sp_alert_queue_shutdown(&app.alert_queue);

    pthread_join(app.sample_thread, NULL);
    pthread_join(app.process_thread, NULL);
    pthread_join(app.alert_thread, NULL);

    sp_wal_close(&app.wal);
    sp_input_close(&app.input);
    sp_alert_sender_close(&app.sender);

    return 0;
}
