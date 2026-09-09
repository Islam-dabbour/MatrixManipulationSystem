#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/wait.h>


struct LogMessage {
    char timestamp[64];

    int clientId;
    pid_t processId;

    unsigned long threadId;

    char event[64];
    char operation[64];

    char message[256];
};


int main(){





    return 0; 
}