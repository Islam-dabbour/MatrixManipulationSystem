#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>


int tasksPerWorker = 100;


struct Matrix{

    int rows;
    int columns;
    int *arr;
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

    return matrix;

}

void printMatrix(struct Matrix matrix){

    for (int i = 0; i < matrix.rows; i++) {

        for (int j = 0; j < matrix.columns; j++) {

            //matrix.arr[i * matrix->columns + j] = (rand() % 100) + 1; 
            printf("%d ",matrix.arr[i * matrix.columns + j]);

        }

        printf("\n");

    }
    
}


void freeAllocatedMemory(struct Matrix *matrix){
    free(matrix->arr);
    
}

void matrixMultipication(struct Matrix matrixA, struct Matrix matrixB){

    if (matrixA.columns != matrixB.rows) {
        printf("Cannot multiply matrices: incompatible dimensions.\n");
        return;
    }


    // int totalTasks = matrixA.rows * matrixB.columns;
    // int tasksPerWorker = totalTasks % 10;
    // int workerCount = totalTasks / tasksPerWorker;
    //int workerCount = totalTasks < 4 ? totalTasks : 4;
    //int tasksPerWorker = (totalTasks + workerCount - 1) / workerCount;

    int totalTasks = matrixA.rows * matrixB.columns;
    int workerCount = totalTasks / tasksPerWorker;

    if (totalTasks % tasksPerWorker != 0) {
        workerCount = workerCount + 1;
    }

    if (workerCount == 0) {
        workerCount = 1;
    }

    for (int worker = 0; worker < workerCount; worker++) {
        pid_t pid = fork();

        if (pid == 0) {
            int firstTask = worker * tasksPerWorker;
            int lastTask = firstTask + tasksPerWorker;

            if (lastTask > totalTasks) {
                lastTask = totalTasks;
            }

            for (int task = firstTask; task < lastTask; task++) {
                int row = task / matrixB.columns;
                int column = task % matrixB.columns;
                int result = 0;

                for (int k = 0; k < matrixA.columns; k++) {
                    result += matrixA.arr[row * matrixA.columns + k] *
                              matrixB.arr[k * matrixB.columns + column];
                }

                // printf("Worker %d calculated C[%d][%d] = %d\n",
                //        worker, row, column, result);
            }

            fflush(stdout);
            _exit(0);
        }

        if (pid < 0) {
            perror("fork");
            break;
        }
    }

    for (int worker = 0; worker < workerCount; worker++) {
        wait(NULL);
    }
}

void matrixTransposition(struct Matrix matrix){

    struct Matrix matrixC = createMatrix(matrix.columns, matrix.rows);


    int totalTasks = matrix.rows * matrix.columns;
    int workerCount = totalTasks / tasksPerWorker;

    if (totalTasks % tasksPerWorker != 0) {
        workerCount = workerCount + 1;
    }

    if (workerCount == 0) {
        workerCount = 1;
    }

    for (int worker = 0; worker < workerCount; worker++) {

        pid_t pid = fork();

        if (pid == 0) {

            int firstTask = worker * tasksPerWorker;
            int lastTask = firstTask + tasksPerWorker;

            if (lastTask > totalTasks) {
                lastTask = totalTasks;
            }

            for (int i = firstTask; i < lastTask; i++) {

                int row = i / matrix.columns;
                int column = i % matrix.columns;
                

                for (int j = 0; j < matrixC.columns; j++) {

                    matrixC.arr[i * matrixC.columns + j] = matrix.arr[j * matrix.columns + i];

                }

            }

            fflush(stdout);
            _exit(0);

        }

         if (pid < 0) {
            perror("fork");
            break;
        }


    }

    for (int worker = 0; worker < workerCount; worker++) {
        wait(NULL);
    }

}

void matrixAverage(struct Matrix matrix){

    double avg = 0.0;
    int sum = 0;


    int totalTasks = matrix.rows * matrix.columns;
    int workerCount = totalTasks / tasksPerWorker;

    if (totalTasks % tasksPerWorker != 0) {
        workerCount = workerCount + 1;
    }

    if (workerCount == 0) {
        workerCount = 1;
    }

    for (int worker = 0; worker < workerCount; worker++) {

        pid_t pid = fork();

        if (pid == 0) {

            int firstTask = worker * tasksPerWorker;
            int lastTask = firstTask + tasksPerWorker;

            if (lastTask > totalTasks) {
                lastTask = totalTasks;
            }


            for (int i = firstTask; i < lastTask; i++) {

                for (int j = 0; j < matrix.columns; j++) {

                    sum = sum + matrix.arr[i * matrix.columns + j];

                }

            }

            fflush(stdout);
            _exit(0);
        }

        

    }

    for (int worker = 0; worker < workerCount; worker++) {
        wait(NULL);
    }

    avg = (double)sum / (matrix.columns * matrix.rows);
    //return avg;

}


int main(){

    srand(time(NULL));
    struct timeval start, end; 
    
    
    

    gettimeofday(&start, NULL); 

    printf("============================\n");
    printf(" Matrix Manipulation System \n");
    printf("============================\n");

    int rowsA = (rand() % 100) + 1;
    int columnsB = (rand() % 100) + 1;
    int columnsA = (rand() % 100) + 1;
    int rowsB = columnsA;
    
    

    struct Matrix matrixA = createMatrix(rowsA, columnsA);
    struct Matrix matrixB = createMatrix(rowsB, columnsB);
    fillMatrix(&matrixA);
    fillMatrix(&matrixB);

    //printMatrix(matrixA);
    int pid1 = fork();

    if( pid1 == 0 ){

        struct timeval startM, endM; 

        printf("============================\n");
        printf("   Matrix Multipication     \n");
        //printf("============================\n");

        gettimeofday(&startM, NULL); 
        matrixMultipication(matrixA, matrixB);
        gettimeofday(&endM, NULL); 

        double timeTakenM =(endM.tv_sec - startM.tv_sec) +(endM.tv_usec - startM.tv_usec) / 1000000.0;
        
        printf("\n");

        printf("> Time taken to finish multipcation > %f\n",timeTakenM);

        printf("\n");

        exit(0);

    }else{

        int pid2 = fork();

        if( pid2 == 0 ){

            struct timeval startT, endT; 

            printf("============================\n");
            printf("   Matrix Transposition     \n");
            //printf("============================\n");

            gettimeofday(&startT, NULL); 
            matrixTransposition(matrixA);
            gettimeofday(&endT, NULL);
            
            double timeTakenT =(endT.tv_sec - startT.tv_sec) +(endT.tv_usec - startT.tv_usec) / 1000000.0;
            
            printf("\n");

            printf("> Time taken to finish Transposition > %f\n",timeTakenT);

            printf("\n");

            //freeAllocatedMemory(&matrixTranspositionResult);
            exit(0);

        }else{
            
            int pid3 = fork();

            if( pid3 == 0 ){

                struct timeval startA, endA; 

                printf("============================\n");
                printf("      Matrix Average        \n");
                //printf("============================\n");

                gettimeofday(&startA, NULL); 
                matrixAverage(matrixA);
                gettimeofday(&endA, NULL);

                double timeTakenA =(endA.tv_sec - startA.tv_sec) +(endA.tv_usec - startA.tv_usec) / 1000000.0;
                
                printf("\n");

                printf("> Time taken to finish Average calculation > %f\n",timeTakenA);
                printf("============================\n");

                exit(0);

            }
        }

    }
    

  

    for ( int i = 0; i < 3; i++){
        
        wait(NULL);
        
    }

    freeAllocatedMemory(&matrixA);
    freeAllocatedMemory(&matrixB);
    
    
    

    gettimeofday(&end, NULL); 
    double timeTaken =(end.tv_sec - start.tv_sec) +(end.tv_usec - start.tv_usec) / 1000000.0;
    printf("\n");

    printf("> Time taken to finish the whole program > %f\n",timeTaken);

    return 0;
}