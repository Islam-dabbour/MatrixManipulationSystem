#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "logger.h"

void log_event(
    int pipe_fd,
    const char *timestamp,
    int client_id,
    const char *event,
    const char *operation,
    const char *message
)

{
    struct LogMessage log = {0};

    snprintf(log.timestamp,sizeof(log.timestamp),"%s",timestamp);

    log.clientId = client_id;

    snprintf(log.event,sizeof(log.event),"%s",event);

    snprintf(log.operation,sizeof(log.operation),"%s",operation );

    snprintf(log.message,sizeof(log.message),"%s",message);

    ssize_t bytes_written = write(pipe_fd,&log,sizeof(log));

    if (bytes_written == -1) {
        perror("write log");
    }
}