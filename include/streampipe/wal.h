#ifndef STREAMPIPE_WAL_H
#define STREAMPIPE_WAL_H

#include "streampipe/common.h"
#include <stdio.h>

#define SP_WAL_MAGIC 0x53545050
#define SP_WAL_VERSION 1

typedef enum {
    SP_WAL_EVENT_ALERT = 1
} sp_wal_event_type_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t type;
    uint64_t ts_ns;
    uint32_t sensor_id;
    int32_t value_fp;
    int32_t mean_fp;
    int32_t std_fp;
    int32_t z_fp;
    uint8_t level;
    uint8_t reserved[3];
    uint32_t crc32;
} sp_wal_record_t;

typedef struct {
    FILE* f;
    uint32_t pending;
    const char* path;
} sp_wal_t;

int sp_wal_open(sp_wal_t* wal, const char* path);
int sp_wal_append_alert(sp_wal_t* wal, const sp_alert_t* alert);
void sp_wal_close(sp_wal_t* wal);

#endif
