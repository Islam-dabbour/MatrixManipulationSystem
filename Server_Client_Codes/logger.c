#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "logger.h"



int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pipe_fd>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int log_pipe_read = atoi(argv[1]);

    FILE *log_file = fopen("server.log", "a");

    if (log_file == NULL) {
        perror("fopen server.log");
        close(log_pipe_read);
        return EXIT_FAILURE;
    }

    printf("[LOGGER] Logger process started. PID = %d\n", getpid());

    struct LogMessage log;

    while (1) {

        ssize_t bytes_read = read(
            log_pipe_read,
            &log,
            sizeof(struct LogMessage)
        );

        if (bytes_read == 0) {
            /*
             * The server closed the pipe.
             * This means the server is shutting down.
             */
            printf("[LOGGER] Logging pipe closed.\n");
            break;
        }

        if (bytes_read < 0) {
            perror("[LOGGER] read");
            break;
        }

        if (bytes_read != sizeof(struct LogMessage)) {
            fprintf(
                stderr,
                "[LOGGER] Incomplete log message received.\n"
            );
            continue;
        }

        fprintf(
            log_file,
            "[%s] [Client:%d] "
            "[%s] [%s] %s\n",
            log.timestamp,
            log.clientId,
            log.event,
            log.operation,
            log.message
        );

        fflush(log_file);
    }

    fclose(log_file);
    close(log_pipe_read);

    printf("[LOGGER] Logger process terminated.\n");

    return 0;
}