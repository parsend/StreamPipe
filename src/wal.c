#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "streampipe/wal.h"

uint32_t sp_crc32(const uint8_t* data, size_t len);

int sp_wal_open(sp_wal_t* wal, const char* path) {
    memset(wal, 0, sizeof(*wal));
    wal->path = path;
    wal->f = fopen(path, "ab");
    if (!wal->f) {
        return -1;
    }

    return 0;
}

int sp_wal_append_alert(sp_wal_t* wal, const sp_alert_t* alert) {
    if (!wal->f || !alert) {
        return -1;
    }

    sp_wal_record_t rec;
    rec.magic = SP_WAL_MAGIC;
    rec.version = SP_WAL_VERSION;
    rec.type = SP_WAL_EVENT_ALERT;
    rec.ts_ns = alert->ts_ns;
    rec.sensor_id = alert->sensor_id;
    rec.value_fp = alert->value_fp;
    rec.mean_fp = alert->mean_fp;
    rec.std_fp = alert->std_fp;
    rec.z_fp = alert->z_fp;
    rec.level = alert->level;
    rec.reserved[0] = 0;
    rec.reserved[1] = 0;
    rec.reserved[2] = 0;

    rec.crc32 = 0;
    rec.crc32 = sp_crc32((const uint8_t*) &rec, sizeof(rec) - sizeof(rec.crc32));

    size_t written = fwrite(&rec, 1, sizeof(rec), wal->f);
    if (written != sizeof(rec)) {
        return -1;
    }

    wal->pending++;
    if (wal->pending >= 32) {
        fflush(wal->f);
        int fd = fileno(wal->f);
        if (fd >= 0) {
            fsync(fd);
        }
        wal->pending = 0;
    }

    return 0;
}

void sp_wal_close(sp_wal_t* wal) {
    if (!wal->f) {
        return;
    }

    fflush(wal->f);
    int fd = fileno(wal->f);
    if (fd >= 0) {
        fsync(fd);
    }

    fclose(wal->f);
    wal->f = NULL;
}
