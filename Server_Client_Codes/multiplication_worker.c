#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TASKS_PER_WORKER 100

struct Matrix{

    int rows;
    int columns;
    int *arr;
};

struct MatricesArgs{
    struct Matrix *matrixA;
    struct Matrix *matrixB;
    struct Matrix result;
    struct Matrix transposedMatrix;
};

struct MatrixMultiplicationTaskArgs{
    const struct Matrix *matrixA;
    const struct Matrix *matrixB;
    struct Matrix *result;
    int firstTask;
    int lastTask;
    pthread_mutex_t *resultMutex;
};

struct Matrix fillMatrix(struct Matrix *matrix){
    for (int i = 0; i < matrix->rows; i++) {

        for (int j = 0; j < matrix->columns; j++) {

            matrix->arr[i * matrix->columns + j] = (rand() % 100) + 1; 

        }

    }

    return *matrix;

}

struct Matrix createMatrix(int rows, int columns){

    
    struct Matrix matrix = {
        .rows = rows,
        .columns = columns,
        .arr = calloc((size_t)rows * columns, sizeof *matrix.arr)
    };

    matrix = fillMatrix(&matrix);

    return matrix;

}


void freeAllocatedMemory(struct Matrix *matrix){
   
    free(matrix->arr);
    
}

struct Matrix createEmptyMatrix(int rows, int columns){

    struct Matrix matrix = {
        .rows = rows,
        .columns = columns,
        .arr = calloc((size_t)rows * columns, sizeof *matrix.arr)
    };

    return matrix;
}


void *matrixMultiplicationTask(void *arg){

    struct MatrixMultiplicationTaskArgs *task = arg;

    for (int outputIndex = task->firstTask; outputIndex < task->lastTask; outputIndex++) {
        int row = outputIndex / task->result->columns;
        int column = outputIndex % task->result->columns;
        int value = 0;

        for (int k = 0; k < task->matrixA->columns; k++) {
            value += task->matrixA->arr[row * task->matrixA->columns + k] *
                     task->matrixB->arr[k * task->matrixB->columns + column];
        }

        pthread_mutex_lock(task->resultMutex);
        task->result->arr[outputIndex] = value;
        pthread_mutex_unlock(task->resultMutex);
    }

    return NULL;
}

struct Matrix matrixMultipication(struct Matrix matrixA, struct Matrix matrixB){

    if (matrixA.columns != matrixB.rows) {
        return (struct Matrix){0};
    }

    struct Matrix matrixC = createEmptyMatrix(matrixA.rows, matrixB.columns);
    int totalTasks = matrixC.rows * matrixC.columns;
    int workerCount = totalTasks > 0 ? (totalTasks + TASKS_PER_WORKER - 1) / TASKS_PER_WORKER : 1;
    pthread_t workers[workerCount];
    struct MatrixMultiplicationTaskArgs taskArgs[workerCount];
    pthread_mutex_t resultMutex;
    int createdWorkers = 0;

    if (pthread_mutex_init(&resultMutex, NULL) != 0) {
        freeAllocatedMemory(&matrixC);
        return (struct Matrix){0};
    }

    for (int worker = 0; worker < workerCount; worker++) {
        int firstTask = worker * TASKS_PER_WORKER;
        int lastTask = firstTask + TASKS_PER_WORKER;

        if (lastTask > totalTasks) {
            lastTask = totalTasks;
        }

        taskArgs[worker] = (struct MatrixMultiplicationTaskArgs){
            .matrixA = &matrixA,
            .matrixB = &matrixB,
            .result = &matrixC,
            .firstTask = firstTask,
            .lastTask = lastTask,
            .resultMutex = &resultMutex
        };

        if (pthread_create(&workers[worker], NULL, matrixMultiplicationTask, &taskArgs[worker]) != 0) {
            fprintf(stderr, "Failed to create multiplication worker thread\n");
            break;
        }

        createdWorkers++;
    }

    for (int worker = 0; worker < createdWorkers; worker++) {
        pthread_join(workers[worker], NULL);
    }

    pthread_mutex_destroy(&resultMutex);

    if (createdWorkers != workerCount) {
        freeAllocatedMemory(&matrixC);
        return (struct Matrix){0};
    }

    return matrixC;
}

void *matrixMultipicationThread (void *arg){

    struct MatricesArgs *matrices = arg;



    return NULL;
}


int main(void){

    srand((unsigned)time(NULL));
   
    mkfifo("multiplication_response", 0666);
    mkfifo("multiplication_request", 0666);

    

    

    // printf("============================\n");
    // printf(" Matrix Manipulation System \n");
    // printf("============================\n");

    int rowsA ;
    int columnsB;
    int columnsA;
    int rowsB;
    
    
    struct Matrix matrixA;
    struct Matrix matrixB;
    struct Matrix matrixC;


    struct timeval startM, endM; 
    int fd = open("multiplication_response", O_WRONLY);
    int fd2 = open("multiplication_request", O_RDONLY);
    read(fd2,&rowsA,sizeof(int));
    read(fd2,&columnsA,sizeof(int));
    read(fd2,matrixA.arr,(size_t)rowsA * columnsA * sizeof matrixA.arr[0]);

    read(fd2,&rowsB,sizeof(int));
    read(fd2,&columnsB,sizeof(int));
    read(fd2,matrixB.arr,(size_t)rowsB * columnsB * sizeof matrixB.arr[0]);

    matrixA = createMatrix(rowsA, columnsA);
    matrixB = createMatrix(rowsB, columnsB);
    matrixC = createEmptyMatrix(rowsA, columnsB);
    // printf("============================\n");
    // printf("   Matrix Multipication     \n");
    //printf("============================\n");
    gettimeofday(&startM, NULL); 
    
    matrixC = matrixMultipication(matrixA,matrixB);


    write(fd,matrixC.arr,(size_t)rowsA * columnsB * sizeof matrixC.arr[0]);

    gettimeofday(&endM, NULL); 
    double timeTakenM =(endM.tv_sec - startM.tv_sec) +(endM.tv_usec - startM.tv_usec) / 1000000.0;
    printf("\n");
    //printf("> Time taken to finish multipcation > %f\n",timeTakenM);

    printf("\n");

    


    

    freeAllocatedMemory(&matrixA);
    freeAllocatedMemory(&matrixB);
    freeAllocatedMemory(&matrixC);
    
    


    printf("\n");

    //printf("> Time taken to finish the whole program > %f\n",timeTaken);

    return 0;
}
