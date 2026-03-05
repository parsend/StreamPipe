#ifndef STREAMPIPE_ALERT_H
#define STREAMPIPE_ALERT_H

#include <netinet/in.h>
#include "streampipe/common.h"

typedef struct {
    int fd;
    bool enabled;
    struct sockaddr_in address;
} sp_alert_sender_t;

int sp_alert_sender_init(sp_alert_sender_t* sender, const char* host, uint16_t port);
int sp_alert_sender_send(const sp_alert_sender_t* sender, const sp_alert_t* alert);
void sp_alert_sender_close(sp_alert_sender_t* sender);

#endif
