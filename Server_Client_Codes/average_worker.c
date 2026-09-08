#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TASKS_PER_WORKER 100

struct Matrix {
    int rows;
    int columns;
    int *arr;
};


struct MatrixAverageTaskArgs {
    const struct Matrix *matrix;

    int firstTask;
    int lastTask;

    long long *totalSum;

    pthread_mutex_t *sumMutex;
};


struct Matrix createEmptyMatrix(int rows, int columns)
{
    struct Matrix matrix = {
        .rows = rows,
        .columns = columns,
        .arr = calloc(
            (size_t)rows * columns,
            sizeof *matrix.arr
        )
    };

    if (matrix.arr == NULL) {

        perror("calloc");

        exit(EXIT_FAILURE);
    }

    return matrix;
}


void freeAllocatedMemory(struct Matrix *matrix)
{
    free(matrix->arr);
}


void *matrixAverageTask(void *arg)
{
    struct MatrixAverageTaskArgs *task = arg;


    long long partialSum = 0;


    for (
        int inputIndex = task->firstTask;
        inputIndex < task->lastTask;
        inputIndex++
    ) {

        partialSum +=
            task->matrix->arr[inputIndex];
    }


    pthread_mutex_lock(task->sumMutex);

    *task->totalSum += partialSum;

    pthread_mutex_unlock(task->sumMutex);


    return NULL;
}


double matrixAverage(struct Matrix matrix)
{
    int totalTasks =
        matrix.rows * matrix.columns;


    if (totalTasks == 0) {
        return 0.0;
    }


    int workerCount =
        totalTasks > 0
            ? (totalTasks + TASKS_PER_WORKER - 1)
                / TASKS_PER_WORKER
            : 1;


    pthread_t workers[workerCount];

    struct MatrixAverageTaskArgs
        taskArgs[workerCount];


    pthread_mutex_t sumMutex;


    long long totalSum = 0;


    int createdWorkers = 0;


    if (
        pthread_mutex_init(
            &sumMutex,
            NULL
        ) != 0
    ) {

        return 0.0;
    }


    for (
        int worker = 0;
        worker < workerCount;
        worker++
    ) {

        int firstTask =
            worker * TASKS_PER_WORKER;


        int lastTask =
            firstTask + TASKS_PER_WORKER;


        if (lastTask > totalTasks) {

            lastTask = totalTasks;
        }


        taskArgs[worker] =
            (struct MatrixAverageTaskArgs){

                .matrix = &matrix,

                .firstTask = firstTask,

                .lastTask = lastTask,

                .totalSum = &totalSum,

                .sumMutex = &sumMutex
            };


        if (
            pthread_create(
                &workers[worker],
                NULL,
                matrixAverageTask,
                &taskArgs[worker]
            ) != 0
        ) {

            fprintf(
                stderr,
                "Failed to create average worker thread\n"
            );

            break;
        }


        createdWorkers++;
    }


    for (
        int worker = 0;
        worker < createdWorkers;
        worker++
    ) {

        pthread_join(
            workers[worker],
            NULL
        );
    }


    pthread_mutex_destroy(&sumMutex);


    if (
        createdWorkers != workerCount
    ) {

        return 0.0;
    }


    return (double)totalSum / totalTasks;
}


int main(int argc, char **argv)
{
    if (argc != 2) {

        fprintf(
            stderr,
            "Usage: %s <client_id>\n",
            argv[0]
        );

        return EXIT_FAILURE;
    }


    int client_id =
        atoi(argv[1]);


    char request_fifo[100];

    char response_fifo[100];


    snprintf(
        request_fifo,
        sizeof(request_fifo),
        "average_request_%d",
        client_id
    );


    snprintf(
        response_fifo,
        sizeof(response_fifo),
        "average_response_%d",
        client_id
    );


    printf(
        "[Average Worker %d] Starting.\n",
        client_id
    );


    int fd =
        open(
            response_fifo,
            O_WRONLY
        );


    if (fd < 0) {

        perror("open response FIFO");

        return EXIT_FAILURE;
    }


    int fd2 =
        open(
            request_fifo,
            O_RDONLY
        );


    if (fd2 < 0) {

        perror("open request FIFO");

        close(fd);

        return EXIT_FAILURE;
    }


    int rows;

    int columns;


    read(
        fd2,
        &rows,
        sizeof(int)
    );


    read(
        fd2,
        &columns,
        sizeof(int)
    );


    struct Matrix matrix =
        createEmptyMatrix(
            rows,
            columns
        );


    read(
        fd2,
        matrix.arr,
        (size_t)rows *
        columns *
        sizeof matrix.arr[0]
    );


    printf(
        "[Average Worker %d] Received %d x %d matrix.\n",
        client_id,
        rows,
        columns
    );


    struct timeval start, end;


    gettimeofday(
        &start,
        NULL
    );


    double average =
        matrixAverage(matrix);


    gettimeofday(
        &end,
        NULL
    );


    double timeTaken =
        (end.tv_sec - start.tv_sec)
        +
        (end.tv_usec - start.tv_usec)
            / 1000000.0;


    write(
        fd,
        &average,
        sizeof(double)
    );


    printf(
        "[Average Worker %d] Completed.\n",
        client_id
    );


    printf(
        "[Average Worker %d] Average: %f\n",
        client_id,
        average
    );


    printf(
        "[Average Worker %d] Time: %f seconds\n",
        client_id,
        timeTaken
    );


    freeAllocatedMemory(&matrix);


    close(fd);

    close(fd2);


    return EXIT_SUCCESS;
}