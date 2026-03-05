#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L
#endif

#include <string.h>

#include "streampipe/filter.h"

static int32_t sp_sqrt_fp(int64_t value_fp) {
    if (value_fp <= 0) {
        return 0;
    }

    int64_t x = value_fp;
    if (x < SP_FP_SCALE) {
        x = SP_FP_SCALE;
    }

    for (int i = 0; i < 24; i++) {
        int64_t q = (value_fp << SP_FP_SHIFT) / x;
        int64_t next = (x + q) >> 1;
        if (next == x || (next > x ? (next - x) : (x - next)) <= 1) {
            return (int32_t) next;
        }
        x = next;
    }

    return (int32_t) x;
}

static sp_sensor_state_t* sp_filter_state_get(sp_filter_t* filter, uint32_t sensor_id) {
    for (int i = 0; i < SP_MAX_SENSORS; i++) {
        if (filter->states[i].active && filter->states[i].sensor_id == sensor_id) {
            return &filter->states[i];
        }
    }

    for (int i = 0; i < SP_MAX_SENSORS; i++) {
        if (!filter->states[i].active) {
            filter->states[i].active = true;
            filter->states[i].sensor_id = sensor_id;
            filter->states[i].head = 0;
            filter->states[i].count = 0;
            filter->states[i].sum = 0;
            filter->states[i].sumsq = 0;
            memset(filter->states[i].values, 0, sizeof(filter->states[i].values));
            return &filter->states[i];
        }
    }

    return NULL;
}

void sp_filter_init(sp_filter_t* filter, const sp_config_t* cfg) {
    memset(filter, 0, sizeof(*filter));
    filter->cfg = cfg;
}

bool sp_filter_process(sp_filter_t* filter, const sp_sample_t* sample, sp_alert_t* alert) {
    if (!filter->cfg || sample == NULL || alert == NULL) {
        return false;
    }

    sp_sensor_state_t* state = sp_filter_state_get(filter, sample->sensor_id);
    if (state == NULL) {
        return false;
    }

    int32_t sample_fp = sample->value_fp;

    if (state->count >= filter->cfg->window_size) {
        int32_t old = state->values[state->head];
        state->sum -= old;
        state->sumsq -= sp_fp_mul(old, old);
    } else {
        state->count++;
    }

    state->values[state->head] = sample_fp;
    state->head = (state->head + 1) % filter->cfg->window_size;
    state->sum += sample_fp;
    state->sumsq += sp_fp_mul(sample_fp, sample_fp);

    if (state->count < 2) {
        return false;
    }

    int32_t mean_fp = (int32_t) (state->sum / state->count);
    int64_t mean_sq = ((int64_t) mean_fp * (int64_t) mean_fp) >> SP_FP_SHIFT;
    int64_t var_fp = (state->sumsq / state->count) - mean_sq;
    if (var_fp < 0) {
        var_fp = 0;
    }

    int32_t std_fp = sp_sqrt_fp(var_fp);
    if (std_fp == 0) {
        return false;
    }

    int32_t z_fp = (int32_t) (((int64_t) (sample_fp - mean_fp) << SP_FP_SHIFT) / std_fp);
    int32_t abs_z_fp = sp_fp_abs(z_fp);

    if (abs_z_fp >= filter->cfg->anomaly_threshold_fp) {
        alert->ts_ns = sample->ts_ns;
        alert->sensor_id = sample->sensor_id;
        alert->value_fp = sample->value_fp;
        alert->mean_fp = mean_fp;
        alert->std_fp = std_fp;
        alert->z_fp = z_fp;
        alert->level = abs_z_fp >= (filter->cfg->anomaly_threshold_fp << 1) ? 2 : 1;
        return true;
    }

    return false;
}
