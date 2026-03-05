#ifndef STREAMPIPE_QUEUE_H
#define STREAMPIPE_QUEUE_H

#include "streampipe/common.h"
#include <pthread.h>
#include <stddef.h>

typedef struct {
    sp_sample_t data[SP_QUEUE_SAMPLE_CAP];
    size_t head;
    size_t tail;
    size_t count;
    const volatile sig_atomic_t* stop_requested;
    pthread_mutex_t mutex;
    pthread_cond_t can_push;
    pthread_cond_t can_pop;
} sp_sample_queue_t;

typedef struct {
    sp_alert_t data[SP_QUEUE_ALERT_CAP];
    size_t head;
    size_t tail;
    size_t count;
    const volatile sig_atomic_t* stop_requested;
    pthread_mutex_t mutex;
    pthread_cond_t can_push;
    pthread_cond_t can_pop;
} sp_alert_queue_t;

void sp_sample_queue_init(sp_sample_queue_t* queue, const volatile sig_atomic_t* stop_requested);
void sp_alert_queue_init(sp_alert_queue_t* queue, const volatile sig_atomic_t* stop_requested);
bool sp_sample_queue_push(sp_sample_queue_t* queue, const sp_sample_t* sample);
bool sp_alert_queue_push(sp_alert_queue_t* queue, const sp_alert_t* alert);
bool sp_sample_queue_pop(sp_sample_queue_t* queue, sp_sample_t* sample);
bool sp_alert_queue_pop(sp_alert_queue_t* queue, sp_alert_t* alert);
void sp_sample_queue_shutdown(sp_sample_queue_t* queue);
void sp_alert_queue_shutdown(sp_alert_queue_t* queue);

#endif
