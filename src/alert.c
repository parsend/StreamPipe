#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "streampipe/alert.h"

static void sp_fp_to_text(int32_t fp, char* out, size_t out_size) {
    int64_t v = fp;
    int sign = 0;
    if (v < 0) {
        v = -v;
        sign = 1;
    }

    int64_t integer = v >> SP_FP_SHIFT;
    int64_t frac = ((v & (SP_FP_SCALE - 1)) * 10000) / SP_FP_SCALE;

    if (sign) {
        snprintf(out, out_size, "-%lld.%04lld", (long long) integer, (long long) frac);
    } else {
        snprintf(out, out_size, "%lld.%04lld", (long long) integer, (long long) frac);
    }
}

int sp_alert_sender_init(sp_alert_sender_t* sender, const char* host, uint16_t port) {
    memset(sender, 0, sizeof(*sender));

    if (!host || host[0] == '\0') {
        sender->enabled = false;
        sender->fd = -1;
        return 0;
    }

    sender->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sender->fd < 0) {
        return -1;
    }

    memset(&sender->address, 0, sizeof(sender->address));
    sender->address.sin_family = AF_INET;
    sender->address.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &sender->address.sin_addr) != 1) {
        close(sender->fd);
        sender->fd = -1;
        return -1;
    }

    sender->enabled = true;
    return 0;
}

int sp_alert_sender_send(const sp_alert_sender_t* sender, const sp_alert_t* alert) {
    if (!sender->enabled || sender->fd < 0 || !alert) {
        return 0;
    }

    char val[32];
    char mean[32];
    char std[32];
    char z[32];

    sp_fp_to_text(alert->value_fp, val, sizeof(val));
    sp_fp_to_text(alert->mean_fp, mean, sizeof(mean));
    sp_fp_to_text(alert->std_fp, std, sizeof(std));
    sp_fp_to_text(alert->z_fp, z, sizeof(z));

    char payload[256];
    int len = snprintf(
        payload,
        sizeof(payload),
        "{\"event\":\"anomaly\",\"sensor\":%u,\"ts_ns\":%llu,\"value\":%s,\"mean\":%s,\"std\":%s,\"z\":%s,\"level\":%u}\n",
        alert->sensor_id,
        (unsigned long long) alert->ts_ns,
        val,
        mean,
        std,
        z,
        alert->level
    );

    if (len <= 0) {
        return -1;
    }

    ssize_t sent = sendto(sender->fd, payload, (size_t) len, 0, (struct sockaddr*) &sender->address, sizeof(sender->address));
    if (sent < 0) {
        return -1;
    }

    return 0;
}

void sp_alert_sender_close(sp_alert_sender_t* sender) {
    if (sender->fd >= 0) {
        close(sender->fd);
        sender->fd = -1;
    }
    sender->enabled = false;
}
