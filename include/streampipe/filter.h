#ifndef STREAMPIPE_FILTER_H
#define STREAMPIPE_FILTER_H

#include "streampipe/common.h"

typedef struct {
    bool active;
    uint32_t sensor_id;
    uint16_t head;
    uint16_t count;
    int64_t sum;
    int64_t sumsq;
    int32_t values[SP_MAX_WINDOW];
} sp_sensor_state_t;

typedef struct {
    const sp_config_t* cfg;
    sp_sensor_state_t states[SP_MAX_SENSORS];
} sp_filter_t;

void sp_filter_init(sp_filter_t* filter, const sp_config_t* cfg);
bool sp_filter_process(sp_filter_t* filter, const sp_sample_t* sample, sp_alert_t* alert);

#endif
