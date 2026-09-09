#ifndef LOGGER_H
#define LOGGER_H

#include <sys/types.h>

struct LogMessage {
    char timestamp[64];

    int clientId;


    char event[64];
    char operation[64];

    char message[256];
};

void log_event(
    int pipe_fd,
    const char *timestamp,
    int client_id,
    const char *event,
    const char *operation,
    const char *message
);

#endif