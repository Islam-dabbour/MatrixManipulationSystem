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
#include "logger.h"

static int next_client_id = 1;
pthread_mutex_t client_id_mutex = PTHREAD_MUTEX_INITIALIZER;
static int log_pipe_write = -1;
static int time_request_write = -1;
static int time_response_read = -1;

struct Matrix{

    int rows;
    int columns;
    int *arr;
};

struct ClientInfo {
    int socket;
    int client_id;
};



struct Matrix createMatrix(int rows, int columns){

    
    struct Matrix matrix = {
        .rows = rows,
        .columns = columns,
        .arr = calloc((size_t)rows * columns, sizeof *matrix.arr)
    };

    return matrix;

}

void freeAllocatedMemory(struct Matrix *matrix){
   
    free(matrix->arr);
    
}

void error(const char *msg){
    perror(msg);
    exit(EXIT_FAILURE);
}

void *handle_client(void *socket_pointer){

    struct ClientInfo *client = socket_pointer;

    int client_sock = client->socket;
    int client_id = client->client_id;

    free(client);

    printf("[+] Client %d connected.\n", client_id);
    log_event(log_pipe_write, time_request_write, time_response_read,
              client_id, "CLIENT_CONNECTED", "SYSTEM", "Client connected");

        char request_fifo[100];
    char response_fifo[100];

    snprintf( request_fifo, sizeof(request_fifo), "multiplication_request_%d",client_id );

    snprintf(response_fifo,sizeof(response_fifo),"multiplication_response_%d", client_id);

    write(client_sock, &client_id, sizeof(client_id));

    while(1){
       int option;
       printf("[Client %d] Waiting for request...\n", client_id);
       read(client_sock, &option, sizeof(int));

       struct Matrix matrixA;
       struct Matrix matrixB;
       int rowsA = 0, columnsB = 0, columnsA = 0, rowsB = 0;

       switch (option)
                {
                case 1:
                    log_event(log_pipe_write, time_request_write,
                              time_response_read, client_id,
                              "REQUEST_RECEIVED", "MULTIPLICATION",
                              "Multiplication request received");
                   
                    read(client_sock,&rowsA, sizeof(int));
                    read(client_sock,&columnsA, sizeof(int));
                    matrixA = createMatrix(rowsA, columnsA);
                    read(client_sock, matrixA.arr, (size_t)rowsA * columnsA * sizeof matrixA.arr[0]);

                    read(client_sock,&rowsB, sizeof(int));
                    read(client_sock,&columnsB, sizeof(int));
                    matrixB = createMatrix(rowsB, columnsB);
                    read(client_sock, matrixB.arr, (size_t)rowsB * columnsB * sizeof matrixA.arr[0]);

                    printf("[Client %d] Multiplication request: " "%d x %d * %d x %d\n", client_id, rowsA,columnsA,rowsB,columnsB);
                    
                    unlink(request_fifo);
                    unlink(response_fifo);

                    if (mkfifo(request_fifo, 0666) < 0) {
                        perror("mkfifo request");
                        log_event(log_pipe_write, time_request_write,
                                  time_response_read, client_id, "ERROR",
                                  "MULTIPLICATION", "Request FIFO creation failed");
                        break;
                    }

                    if (mkfifo(response_fifo, 0666) < 0) {
                        perror("mkfifo response");
                        log_event(log_pipe_write, time_request_write,
                                  time_response_read, client_id, "ERROR",
                                  "MULTIPLICATION", "Response FIFO creation failed");
                        unlink(request_fifo);
                        break;
                    }

                    pid_t pid = fork();

                    if (pid < 0) {
                        perror("fork");
                        log_event(log_pipe_write, time_request_write,
                                  time_response_read, client_id, "ERROR",
                                  "MULTIPLICATION", "Worker creation failed");

                        unlink(request_fifo);
                        unlink(response_fifo);

                        freeAllocatedMemory(&matrixA);
                        freeAllocatedMemory(&matrixB);

                        break;
                    }

                    log_event(log_pipe_write, time_request_write,
                              time_response_read, client_id, "WORKER_CREATED",
                              "MULTIPLICATION", "Multiplication worker started");


                    if (pid == 0)
                    {
    

                        char id_string[20];

                        snprintf(id_string,sizeof(id_string),"%d",client_id);

                        execl("./multiplication_worker","multiplication_worker",id_string,NULL);

                        perror("execl multiplication_worker");
                        exit(EXIT_FAILURE);
                    }

                    int fd = open(response_fifo, O_RDONLY);
                    int fd2 = open(request_fifo, O_WRONLY);

                    write(fd2,&rowsA,sizeof(int));
                    write(fd2,&columnsA,sizeof(int));
                    write(fd2,matrixA.arr,(size_t)rowsA * columnsA * sizeof matrixA.arr[0]);

                    write(fd2,&rowsB,sizeof(int));
                    write(fd2,&columnsB,sizeof(int));
                    write(fd2,matrixB.arr,(size_t)rowsB * columnsB * sizeof matrixB.arr[0]);

                    struct Matrix matrixC1 = createMatrix(rowsA, columnsB);

                    read(fd,matrixC1.arr,(size_t)rowsA * columnsB * sizeof matrixC1.arr[0]);

                    write(client_sock, matrixC1.arr, (size_t)rowsA * columnsB * sizeof matrixC1.arr[0]);

                    printf("[Client %d] Multiplication completed.\n",client_id );
                    log_event(log_pipe_write, time_request_write,
                              time_response_read, client_id, "OPERATION_COMPLETED",
                              "MULTIPLICATION", "Multiplication completed");
                    
                    waitpid(pid, NULL, 0);


        
                    unlink(request_fifo);
                    unlink(response_fifo);

                    freeAllocatedMemory(&matrixA);
                    freeAllocatedMemory(&matrixB);
                    freeAllocatedMemory(&matrixC1);
                    break;
                case 2:
                {
                    log_event(log_pipe_write, time_request_write,
                              time_response_read, client_id,
                              "REQUEST_RECEIVED", "TRANSPOSITION",
                              "Transposition request received");
                    char transpose_request_fifo[100];
                    char transpose_response_fifo[100];
                    snprintf(transpose_request_fifo, sizeof transpose_request_fifo,
                             "transposition_request_%d", client_id);
                    snprintf(transpose_response_fifo, sizeof transpose_response_fifo,
                             "transposition_response_%d", client_id);

                    read(client_sock, &rowsA, sizeof rowsA);
                    read(client_sock, &columnsA, sizeof columnsA);
                    matrixA = createMatrix(rowsA, columnsA);
                    read(client_sock, matrixA.arr,
                         (size_t)rowsA * columnsA * sizeof matrixA.arr[0]);

                    unlink(transpose_request_fifo);
                    unlink(transpose_response_fifo);
                    if (mkfifo(transpose_request_fifo, 0666) < 0 ||
                        mkfifo(transpose_response_fifo, 0666) < 0) {
                        perror("mkfifo transposition");
                        log_event(log_pipe_write, time_request_write,
                                  time_response_read, client_id, "ERROR",
                                  "TRANSPOSITION", "FIFO creation failed");
                        unlink(transpose_request_fifo);
                        unlink(transpose_response_fifo);
                        freeAllocatedMemory(&matrixA);
                        break;
                    }

                    pid_t pid = fork();
                    if (pid < 0) {
                        perror("fork transposition worker");
                        log_event(log_pipe_write, time_request_write,
                                  time_response_read, client_id, "ERROR",
                                  "TRANSPOSITION", "Worker creation failed");
                        unlink(transpose_request_fifo);
                        unlink(transpose_response_fifo);
                        freeAllocatedMemory(&matrixA);
                        break;
                    }

                    log_event(log_pipe_write, time_request_write,
                              time_response_read, client_id, "WORKER_CREATED",
                              "TRANSPOSITION", "Transposition worker started");
                    if (pid == 0) {
                        char id_string[20];
                        snprintf(id_string, sizeof id_string, "%d", client_id);
                        execl("./transposition_worker", "transposition_worker",
                              id_string, NULL);
                        perror("execl transposition_worker");
                        exit(EXIT_FAILURE);
                    }

                    int response_fd = open(transpose_response_fifo, O_RDONLY);
                    int request_fd = open(transpose_request_fifo, O_WRONLY);
                    write(request_fd, &rowsA, sizeof rowsA);
                    write(request_fd, &columnsA, sizeof columnsA);
                    write(request_fd, matrixA.arr,
                          (size_t)rowsA * columnsA * sizeof matrixA.arr[0]);

                    struct Matrix transpose = createMatrix(columnsA, rowsA);
                    read(response_fd, transpose.arr,
                         (size_t)columnsA * rowsA * sizeof transpose.arr[0]);
                    write(client_sock, transpose.arr,
                          (size_t)columnsA * rowsA * sizeof transpose.arr[0]);
                      log_event(log_pipe_write, time_request_write,
                            time_response_read, client_id, "OPERATION_COMPLETED",
                            "TRANSPOSITION", "Transposition completed");

                    close(request_fd);
                    close(response_fd);
                    waitpid(pid, NULL, 0);
                    unlink(transpose_request_fifo);
                    unlink(transpose_response_fifo);
                    freeAllocatedMemory(&matrixA);
                    freeAllocatedMemory(&transpose);
                    break;
                }
                case 3:
                    log_event(log_pipe_write, time_request_write,
                              time_response_read, client_id,
                              "REQUEST_RECEIVED", "MATRIX_INPUT",
                              "Matrix input request received");
                    read(client_sock,&rowsB, sizeof(int));
                    read(client_sock,&columnsB, sizeof(int));
                    matrixA = createMatrix(rowsB, columnsB);
                    read(client_sock, matrixA.arr, (size_t)rowsB * columnsB * sizeof matrixA.arr[0]);


                    break;
                case 4:
                {
                    log_event(log_pipe_write, time_request_write,
                              time_response_read, client_id,
                              "REQUEST_RECEIVED", "AVERAGE",
                              "Average request received");
                    char average_request_fifo[100];
                    char average_response_fifo[100];
                    snprintf(average_request_fifo, sizeof average_request_fifo,
                             "average_request_%d", client_id);
                    snprintf(average_response_fifo, sizeof average_response_fifo,
                             "average_response_%d", client_id);

                    read(client_sock, &rowsA, sizeof rowsA);
                    read(client_sock, &columnsA, sizeof columnsA);
                    matrixA = createMatrix(rowsA, columnsA);
                    read(client_sock, matrixA.arr,
                         (size_t)rowsA * columnsA * sizeof matrixA.arr[0]);

                    unlink(average_request_fifo);
                    unlink(average_response_fifo);
                    if (mkfifo(average_request_fifo, 0666) < 0 ||
                        mkfifo(average_response_fifo, 0666) < 0) {
                        perror("mkfifo average");
                        log_event(log_pipe_write, time_request_write,
                                  time_response_read, client_id, "ERROR",
                                  "AVERAGE", "FIFO creation failed");
                        unlink(average_request_fifo);
                        unlink(average_response_fifo);
                        freeAllocatedMemory(&matrixA);
                        break;
                    }

                    pid_t pid = fork();
                    if (pid < 0) {
                        perror("fork average worker");
                        log_event(log_pipe_write, time_request_write,
                                  time_response_read, client_id, "ERROR",
                                  "AVERAGE", "Worker creation failed");
                        unlink(average_request_fifo);
                        unlink(average_response_fifo);
                        freeAllocatedMemory(&matrixA);
                        break;
                    }

                    log_event(log_pipe_write, time_request_write,
                              time_response_read, client_id, "WORKER_CREATED",
                              "AVERAGE", "Average worker started");
                    if (pid == 0) {
                        char id_string[20];
                        snprintf(id_string, sizeof id_string, "%d", client_id);
                        execl("./average_worker", "average_worker", id_string, NULL);
                        perror("execl average_worker");
                        exit(EXIT_FAILURE);
                    }

                    int response_fd = open(average_response_fifo, O_RDONLY);
                    int request_fd = open(average_request_fifo, O_WRONLY);
                    write(request_fd, &rowsA, sizeof rowsA);
                    write(request_fd, &columnsA, sizeof columnsA);
                    write(request_fd, matrixA.arr,
                          (size_t)rowsA * columnsA * sizeof matrixA.arr[0]);

                    double average = 0.0;
                    read(response_fd, &average, sizeof average);
                    write(client_sock, &average, sizeof average);
                    log_event(log_pipe_write, time_request_write,
                              time_response_read, client_id, "OPERATION_COMPLETED",
                              "AVERAGE", "Average completed");

                    close(request_fd);
                    close(response_fd);
                    waitpid(pid, NULL, 0);
                    unlink(average_request_fifo);
                    unlink(average_response_fifo);
                    freeAllocatedMemory(&matrixA);
                    break;
                }
                case 5:
                    log_event(log_pipe_write, time_request_write,
                              time_response_read, client_id,
                              "REQUEST_RECEIVED", "MATRIX_INPUT",
                              "Matrix input request received");
                    read(client_sock,&rowsB, sizeof(int));
                    read(client_sock,&columnsB, sizeof(int));
                    matrixA = createMatrix(rowsB, columnsB);
                    read(client_sock, matrixA.arr, (size_t)rowsB * columnsB * sizeof matrixA.arr[0]);


                    break;
                case 6:
                    continue;
                default:
                    break;
                }
    }

    close(client_sock);
    printf("[-] Client %d disconnected.\n", client_id);
    log_event(log_pipe_write, time_request_write, time_response_read,
              client_id, "CLIENT_DISCONNECTED", "SYSTEM", "Client disconnected");
    return NULL;
}

int main(int argc, char **argv){

    if(argc != 2){
        printf("You need to provide a port number\n");
        exit(0);
    }
    int log_pipe[2];
    int time_request_pipe[2];
    int time_response_pipe[2];

    if (pipe(log_pipe) < 0 || pipe(time_request_pipe) < 0 ||
        pipe(time_response_pipe) < 0) {
        perror("pipe");
        return EXIT_FAILURE;
    }

    pid_t pid_time = fork();
    if (pid_time < 0) {
        perror("fork time process");
        return EXIT_FAILURE;
    }

    if (pid_time == 0) {
        char request_fd_string[20];
        char response_fd_string[20];

        close(log_pipe[0]);
        close(log_pipe[1]);
        close(time_request_pipe[1]);
        close(time_response_pipe[0]);
        snprintf(request_fd_string, sizeof request_fd_string, "%d",
                 time_request_pipe[0]);
        snprintf(response_fd_string, sizeof response_fd_string, "%d",
                 time_response_pipe[1]);
        execl("./time_process", "time_process", request_fd_string,
              response_fd_string, NULL);
        perror("execl time_process");
        exit(EXIT_FAILURE);
    }

    int pid_logger = fork();

    if(pid_logger == 0){
        close(log_pipe[1]);
        close(time_request_pipe[0]);
        close(time_request_pipe[1]);
        close(time_response_pipe[0]);
        close(time_response_pipe[1]);

        char pipe_fd_string[20];

        snprintf(
            pipe_fd_string,
            sizeof(pipe_fd_string),
            "%d",
            log_pipe[0]
        );

        execl("./logger","logger",pipe_fd_string,NULL);

     
        perror("execl logger");
        exit(EXIT_FAILURE);
    }

    close(log_pipe[0]);
    close(time_request_pipe[0]);
    close(time_response_pipe[1]);
    log_pipe_write = log_pipe[1];
    time_request_write = time_request_pipe[1];
    time_response_read = time_response_pipe[0];

    log_event(log_pipe_write, time_request_write, time_response_read,
              0, "SERVER_STARTED", "SYSTEM", "Server started successfully");

    int port = atoi(argv[1]);

    int server_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    int reuse_address = 1;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if(server_sock < 0){
        perror("Socket Error");
        exit(1);
    }

    printf("[+] Server socket created.\n");

    if(setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR,
                  &reuse_address, sizeof(reuse_address)) < 0)
    {
        error("Setsockopt Error");
    }

    memset(&server_addr, '\0', sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(server_sock,
           (struct sockaddr*)&server_addr,
           sizeof(server_addr)) < 0)
    {
        perror("Bind Error");
        exit(1);
    }

    printf("[+] Bound to port %d\n", port);

    if(listen(server_sock, 5) < 0){
        error("Listen Error");
    }

    printf("Listening. Press Ctrl+C to stop the server.\n");

    while(1){
        pthread_t client_thread;
        struct ClientInfo *client = malloc(sizeof(struct ClientInfo));

        if(client == NULL){
            perror("Memory Allocation Error");
            continue;
        }

        addr_size = sizeof(client_addr);
        client->socket = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size);

        if(client->socket < 0){
            perror("Accept Error");
            free(client);
            continue;
        }

        pthread_mutex_lock(&client_id_mutex);

        client->client_id = next_client_id++;

        pthread_mutex_unlock(&client_id_mutex);

        printf("[+] New client assigned ID: %d\n",client->client_id);

        if(pthread_create(&client_thread, NULL, handle_client, client) != 0){
            perror("Thread Creation Error");
            close(client->socket);
            free(client);
            continue;
        }

        pthread_detach(client_thread);
    }

    close(server_sock);
    return EXIT_SUCCESS;
 }
