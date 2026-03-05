#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "streampipe/input.h"

static int32_t sp_random_next(sp_input_t* input) {
    input->sim_seed = input->sim_seed * 1103515245u + 12345u;
    return (int32_t) (input->sim_seed >> 16);
}

static int32_t sp_sim_read(sp_input_t* input) {
    int32_t wave = (int32_t) ((input->sim_counter * 17) % 2000) - 1000;
    int32_t noise = (sp_random_next(input) % 201) - 100;
    int32_t spike = (sp_random_next(input) % 8000) == 0
        ? (sp_random_next(input) % 2 ? 2500 : -2500)
        : 0;
    input->sim_counter++;
    return wave + noise + spike;
}

static int sp_input_configure_uart(int fd) {
    if (!isatty(fd)) {
        return 0;
    }

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        return -1;
    }

    cfmakeraw(&tty);
    cfsetispeed(&tty, B115200);
    cfsetospeed(&tty, B115200);

    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;

    tty.c_cc[VTIME] = 0;
    tty.c_cc[VMIN] = 1;

    return tcsetattr(fd, TCSANOW, &tty);
}

int sp_input_init(sp_input_t* input, const sp_config_t* cfg) {
    memset(input, 0, sizeof(*input));
    input->fd = -1;
    input->type = cfg->source;
    input->device = cfg->source_device;
    input->sensor_id = cfg->default_sensor_id;
    input->sample_interval_ms = cfg->sample_interval_ms;
    input->sim_counter = 0;
    input->sim_seed = 0x1234567u;

    if (cfg->source == SP_SOURCE_SIM) {
        return 0;
    }

    int flags = O_RDONLY | O_NONBLOCK;
    if (cfg->source == SP_SOURCE_UART) {
        flags |= O_NOCTTY;
    }

    input->fd = open(cfg->source_device, flags);
    if (input->fd < 0) {
        return -1;
    }

    if (cfg->source == SP_SOURCE_UART) {
        if (sp_input_configure_uart(input->fd) != 0) {
            close(input->fd);
            input->fd = -1;
            return -1;
        }
    }

    return 0;
}

void sp_input_close(sp_input_t* input) {
    if (input->fd >= 0) {
        close(input->fd);
        input->fd = -1;
    }
}

static int sp_read_int_line(int fd, int32_t* out) {
    char c;
    int32_t value = 0;
    int sign = 1;
    int started = 0;

    while (1) {
        ssize_t r = read(fd, &c, 1);
        if (r <= 0) {
            return -1;
        }

        if (c == '-') {
            if (!started) {
                sign = -1;
                started = 1;
                continue;
            }
            return -1;
        }

        if (c >= '0' && c <= '9') {
            started = 1;
            value = (value * 10) + (c - '0');
            continue;
        }

        if (started) {
            *out = value * sign;
            return 0;
        }
    }
}

static int sp_read_int32_le(int fd, int32_t* out) {
    uint8_t b[4];
    ssize_t got = read(fd, b, sizeof(b));
    if (got != 4) {
        return -1;
    }

    *out = (int32_t) (
        ((uint32_t) b[0]) |
        (((uint32_t) b[1]) << 8) |
        (((uint32_t) b[2]) << 16) |
        (((uint32_t) b[3]) << 24));
    return 0;
}

int sp_input_read(sp_input_t* input, sp_sample_t* sample) {
    int32_t raw;

    if (input->type == SP_SOURCE_SIM) {
        raw = sp_sim_read(input);
        sample->ts_ns = sp_now_ns();
        sample->sensor_id = input->sensor_id;
        sample->value_fp = sp_fp_from_raw(raw);
        return 0;
    }

    if (input->type == SP_SOURCE_UART) {
        if (sp_read_int_line(input->fd, &raw) != 0) {
            return -1;
        }
        sample->ts_ns = sp_now_ns();
        sample->sensor_id = input->sensor_id;
        sample->value_fp = sp_fp_from_raw(raw);
        return 0;
    }

    if (sp_read_int32_le(input->fd, &raw) != 0) {
        return -1;
    }

    sample->ts_ns = sp_now_ns();
    sample->sensor_id = input->sensor_id;
    sample->value_fp = sp_fp_from_raw(raw);
    return 0;
}
