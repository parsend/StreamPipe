#include <pthread.h>
#include <stdlib.h>

#include "streampipe/queue.h"

void sp_sample_queue_init(sp_sample_queue_t* queue, const volatile sig_atomic_t* stop_requested) {
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->stop_requested = stop_requested;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->can_push, NULL);
    pthread_cond_init(&queue->can_pop, NULL);
}

void sp_alert_queue_init(sp_alert_queue_t* queue, const volatile sig_atomic_t* stop_requested) {
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->stop_requested = stop_requested;
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->can_push, NULL);
    pthread_cond_init(&queue->can_pop, NULL);
}

bool sp_sample_queue_push(sp_sample_queue_t* queue, const sp_sample_t* sample) {
    bool pushed = false;

    pthread_mutex_lock(&queue->mutex);
    while (queue->count == SP_QUEUE_SAMPLE_CAP && !*queue->stop_requested) {
        pthread_cond_wait(&queue->can_push, &queue->mutex);
    }

    if (!*queue->stop_requested) {
        queue->data[queue->tail] = *sample;
        queue->tail = (queue->tail + 1) % SP_QUEUE_SAMPLE_CAP;
        queue->count++;
        pushed = true;
        pthread_cond_signal(&queue->can_pop);
    }

    pthread_mutex_unlock(&queue->mutex);
    return pushed;
}

bool sp_alert_queue_push(sp_alert_queue_t* queue, const sp_alert_t* alert) {
    bool pushed = false;

    pthread_mutex_lock(&queue->mutex);
    while (queue->count == SP_QUEUE_ALERT_CAP && !*queue->stop_requested) {
        pthread_cond_wait(&queue->can_push, &queue->mutex);
    }

    if (!*queue->stop_requested) {
        queue->data[queue->tail] = *alert;
        queue->tail = (queue->tail + 1) % SP_QUEUE_ALERT_CAP;
        queue->count++;
        pushed = true;
        pthread_cond_signal(&queue->can_pop);
    }

    pthread_mutex_unlock(&queue->mutex);
    return pushed;
}

bool sp_sample_queue_pop(sp_sample_queue_t* queue, sp_sample_t* sample) {
    bool popped = false;

    pthread_mutex_lock(&queue->mutex);
    while (queue->count == 0 && !*queue->stop_requested) {
        pthread_cond_wait(&queue->can_pop, &queue->mutex);
    }

    if (queue->count > 0) {
        *sample = queue->data[queue->head];
        queue->head = (queue->head + 1) % SP_QUEUE_SAMPLE_CAP;
        queue->count--;
        popped = true;
        pthread_cond_signal(&queue->can_push);
    }

    pthread_mutex_unlock(&queue->mutex);
    return popped;
}

bool sp_alert_queue_pop(sp_alert_queue_t* queue, sp_alert_t* alert) {
    bool popped = false;

    pthread_mutex_lock(&queue->mutex);
    while (queue->count == 0 && !*queue->stop_requested) {
        pthread_cond_wait(&queue->can_pop, &queue->mutex);
    }

    if (queue->count > 0) {
        *alert = queue->data[queue->head];
        queue->head = (queue->head + 1) % SP_QUEUE_ALERT_CAP;
        queue->count--;
        popped = true;
        pthread_cond_signal(&queue->can_push);
    }

    pthread_mutex_unlock(&queue->mutex);
    return popped;
}

void sp_sample_queue_shutdown(sp_sample_queue_t* queue) {
    pthread_mutex_lock(&queue->mutex);
    pthread_cond_broadcast(&queue->can_pop);
    pthread_cond_broadcast(&queue->can_push);
    pthread_mutex_unlock(&queue->mutex);
}

void sp_alert_queue_shutdown(sp_alert_queue_t* queue) {
    pthread_mutex_lock(&queue->mutex);
    pthread_cond_broadcast(&queue->can_pop);
    pthread_cond_broadcast(&queue->can_push);
    pthread_mutex_unlock(&queue->mutex);
}
