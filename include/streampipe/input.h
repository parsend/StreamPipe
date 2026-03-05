#ifndef STREAMPIPE_INPUT_H
#define STREAMPIPE_INPUT_H

#include "streampipe/common.h"

typedef struct {
    sp_source_type_t type;
    const char* device;
    int fd;
    uint32_t sensor_id;
    uint32_t sample_interval_ms;
    uint32_t sim_counter;
    uint32_t sim_seed;
} sp_input_t;

int sp_input_init(sp_input_t* input, const sp_config_t* cfg);
void sp_input_close(sp_input_t* input);
int sp_input_read(sp_input_t* input, sp_sample_t* sample);

#endif
