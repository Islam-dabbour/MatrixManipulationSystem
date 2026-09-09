#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "logger.h"

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void log_event(
    int pipe_fd,
    int time_request_fd,
    int time_response_fd,
    int client_id,
    const char *event,
    const char *operation,
    const char *message
)

{
    struct LogMessage log = {0};
    char timestamp_request = 1;

    pthread_mutex_lock(&log_mutex);

    if (write(time_request_fd, &timestamp_request,
              sizeof timestamp_request) != sizeof timestamp_request) {
        perror("write timestamp request");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    if (read(time_response_fd, log.timestamp, sizeof log.timestamp) !=
        sizeof log.timestamp) {
        perror("read timestamp response");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    log.clientId = client_id;

    snprintf(log.event,sizeof(log.event),"%s",event);

    snprintf(log.operation,sizeof(log.operation),"%s",operation );

    snprintf(log.message,sizeof(log.message),"%s",message);

    ssize_t bytes_written = write(pipe_fd, &log, sizeof log);

    if (bytes_written != sizeof log) {
        perror("write log");
    }

    pthread_mutex_unlock(&log_mutex);
}