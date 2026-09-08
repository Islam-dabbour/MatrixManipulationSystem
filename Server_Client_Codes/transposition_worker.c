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

struct MatrixTranspositionTaskArgs {
    const struct Matrix *matrix;
    struct Matrix *result;
    int firstTask;
    int lastTask;
    pthread_mutex_t *resultMutex;
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


void *matrixTranspositionTask(void *arg)
{
    struct MatrixTranspositionTaskArgs *task = arg;

    for (
        int inputIndex = task->firstTask;
        inputIndex < task->lastTask;
        inputIndex++
    ) {
        int row =
            inputIndex / task->matrix->columns;

        int column =
            inputIndex % task->matrix->columns;

        int outputIndex =
            column * task->result->columns + row;

        int value =
            task->matrix->arr[inputIndex];

        pthread_mutex_lock(task->resultMutex);

        task->result->arr[outputIndex] = value;

        pthread_mutex_unlock(task->resultMutex);
    }

    return NULL;
}


struct Matrix matrixTransposition(struct Matrix matrix)
{
    struct Matrix matrixC =
        createEmptyMatrix(
            matrix.columns,
            matrix.rows
        );

    int totalTasks =
        matrix.rows * matrix.columns;

    int workerCount =
        totalTasks > 0
            ? (totalTasks + TASKS_PER_WORKER - 1)
                / TASKS_PER_WORKER
            : 1;

    pthread_t workers[workerCount];

    struct MatrixTranspositionTaskArgs
        taskArgs[workerCount];

    pthread_mutex_t resultMutex;

    int createdWorkers = 0;


    if (pthread_mutex_init(&resultMutex, NULL) != 0) {

        freeAllocatedMemory(&matrixC);

        return (struct Matrix){0};
    }


    for (int worker = 0;
         worker < workerCount;
         worker++) {

        int firstTask =
            worker * TASKS_PER_WORKER;

        int lastTask =
            firstTask + TASKS_PER_WORKER;

        if (lastTask > totalTasks) {
            lastTask = totalTasks;
        }


        taskArgs[worker] =
            (struct MatrixTranspositionTaskArgs){

                .matrix = &matrix,

                .result = &matrixC,

                .firstTask = firstTask,

                .lastTask = lastTask,

                .resultMutex = &resultMutex
            };


        if (
            pthread_create(
                &workers[worker],
                NULL,
                matrixTranspositionTask,
                &taskArgs[worker]
            ) != 0
        ) {

            fprintf(
                stderr,
                "Failed to create transposition worker thread\n"
            );

            break;
        }

        createdWorkers++;
    }


    for (int worker = 0;
         worker < createdWorkers;
         worker++) {

        pthread_join(
            workers[worker],
            NULL
        );
    }


    pthread_mutex_destroy(&resultMutex);


    if (createdWorkers != workerCount) {

        freeAllocatedMemory(&matrixC);

        return (struct Matrix){0};
    }


    return matrixC;
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


    int client_id = atoi(argv[1]);


    char request_fifo[100];
    char response_fifo[100];


    snprintf(
        request_fifo,
        sizeof(request_fifo),
        "transposition_request_%d",
        client_id
    );


    snprintf(
        response_fifo,
        sizeof(response_fifo),
        "transposition_response_%d",
        client_id
    );


    printf(
        "[Transposition Worker %d] Starting.\n",
        client_id
    );


    int fd = open(
        response_fifo,
        O_WRONLY
    );

    if (fd < 0) {

        perror("open response FIFO");

        return EXIT_FAILURE;
    }


    int fd2 = open(
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
        (size_t)rows * columns *
        sizeof matrix.arr[0]
    );


    printf(
        "[Transposition Worker %d] Received %d x %d matrix.\n",
        client_id,
        rows,
        columns
    );


    struct timeval start, end;

    gettimeofday(&start, NULL);


    struct Matrix result =
        matrixTransposition(matrix);


    gettimeofday(&end, NULL);


    double timeTaken =
        (end.tv_sec - start.tv_sec)
        +
        (end.tv_usec - start.tv_usec)
            / 1000000.0;


    write(
        fd,
        result.arr,
        (size_t)result.rows *
        result.columns *
        sizeof result.arr[0]
    );


    printf(
        "[Transposition Worker %d] Completed.\n",
        client_id
    );

    printf(
        "[Transposition Worker %d] Time: %f seconds\n",
        client_id,
        timeTaken
    );


    freeAllocatedMemory(&matrix);

    freeAllocatedMemory(&result);


    close(fd);

    close(fd2);


    return EXIT_SUCCESS;
}