#ifndef STREAMPIPE_COMMON_H
#define STREAMPIPE_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#define SP_FP_SHIFT 16
#define SP_FP_SCALE (1 << SP_FP_SHIFT)

#define SP_MAX_WINDOW 128
#define SP_MAX_SENSORS 16

#define SP_QUEUE_SAMPLE_CAP 128
#define SP_QUEUE_ALERT_CAP 64

typedef enum {
    SP_SOURCE_SIM = 0,
    SP_SOURCE_UART,
    SP_SOURCE_SPI,
    SP_SOURCE_I2C
} sp_source_type_t;

typedef struct {
    sp_source_type_t source;
    const char* source_device;
    uint32_t default_sensor_id;
    uint16_t window_size;
    int32_t anomaly_threshold_fp;
    uint32_t sample_interval_ms;
    const char* wal_path;
    const char* alert_host;
    uint16_t alert_port;
} sp_config_t;

typedef struct {
    uint64_t ts_ns;
    uint32_t sensor_id;
    int32_t value_fp;
} sp_sample_t;

typedef struct {
    uint64_t ts_ns;
    uint32_t sensor_id;
    int32_t value_fp;
    int32_t mean_fp;
    int32_t std_fp;
    int32_t z_fp;
    uint8_t level;
} sp_alert_t;

static inline uint64_t sp_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t) ts.tv_sec * 1000000000ull + (uint64_t) ts.tv_nsec;
}

static inline int32_t sp_fp_from_raw(int32_t raw) {
    return raw << SP_FP_SHIFT;
}

static inline int64_t sp_fp_mul(int32_t a, int32_t b) {
    return ((int64_t) a * (int64_t) b) >> SP_FP_SHIFT;
}

static inline int32_t sp_fp_abs(int32_t v) {
    return v < 0 ? -v : v;
}

#endif
