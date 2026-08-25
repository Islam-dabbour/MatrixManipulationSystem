#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>


int tasksPerWorker = 100;
#define MAX_VALUES_PER_CHUNK 100

struct Matrix{

    int rows;
    int columns;
    int *arr;
};

struct TransposeChunk {
    int start;
    int end;
    int outputIndex[MAX_VALUES_PER_CHUNK];
    int values[MAX_VALUES_PER_CHUNK];
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





struct Matrix matrixMultipication(struct Matrix matrixA, struct Matrix matrixB){

    if (matrixA.columns != matrixB.rows) {
        printf("Cannot multiply matrices: incompatible dimensions.\n");
        return createMatrix(0, 0);
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

    struct Matrix matrixC = createMatrix(matrixA.rows, matrixB.columns);
    int resultPipes[workerCount][2];
    pid_t workerPids[workerCount];

    for (int worker = 0; worker < workerCount; worker++) {
        if (pipe(resultPipes[worker]) == -1) {
            perror("pipe");
            freeAllocatedMemory(&matrixC);
            return createMatrix(0, 0);
        }

        pid_t pid = fork();

        if (pid == 0) {
            int firstTask = worker * tasksPerWorker;
            int lastTask = firstTask + tasksPerWorker;

            if (lastTask > totalTasks) {
                lastTask = totalTasks;
            }

            struct TransposeChunk chunk = {
                .start = firstTask,
                .end = lastTask
            };

            for (int task = firstTask; task < lastTask; task++) {
                int row = task / matrixB.columns;
                int column = task % matrixB.columns;
                int result = 0;

                for (int k = 0; k < matrixA.columns; k++) {
                    result += matrixA.arr[row * matrixA.columns + k] *
                              matrixB.arr[k * matrixB.columns + column];
                }

                int chunkIndex = task - firstTask;
                chunk.outputIndex[chunkIndex] = task;
                chunk.values[chunkIndex] = result;
            }

            close(resultPipes[worker][0]);
            write(resultPipes[worker][1], &chunk, sizeof chunk);
            close(resultPipes[worker][1]);
            _exit(0);
        }

        if (pid < 0) {
            perror("fork");
            close(resultPipes[worker][0]);
            close(resultPipes[worker][1]);
            continue;
        }

        workerPids[worker] = pid;
        close(resultPipes[worker][1]);
    }

    for (int worker = 0; worker < workerCount; worker++) {
        struct TransposeChunk chunk;

        if (read(resultPipes[worker][0], &chunk, sizeof chunk) == sizeof chunk) {
            for (int chunkIndex = 0; chunkIndex < chunk.end - chunk.start; chunkIndex++) {
                matrixC.arr[chunk.outputIndex[chunkIndex]] = chunk.values[chunkIndex];
            }
        }

        close(resultPipes[worker][0]);
        waitpid(workerPids[worker], NULL, 0);
    }

    return matrixC;
}

struct Matrix matrixTransposition(struct Matrix matrix){

    // before we made the IPC communication we had to remove this matrix since each process has its 
    // own memeory childs/parents, so i made them in a way where each calculate the result and the task 
    // without sharing
    // but now we need this matrix so each child write on the exact index he worked at
    // and then send it to the parent to join it with the rest of the result in one main matrix

    // and since its transposition if the original matrix is rows * columns the trnasposed will be:
    struct Matrix matrixC = createMatrix(matrix.columns, matrix.rows);

    // here we calculate the number of workers, based on the custom number of tasks per woekr
    // that i made as global varialbe, so each child will work with 100 elements/tasks
    int totalTasks = matrix.rows * matrix.columns;
    int workerCount = totalTasks / tasksPerWorker;

    // if the number of total tasks isnt even (there is an extra worker needed) it adds an extra worker
    if (totalTasks % tasksPerWorker != 0) {
        workerCount = workerCount + 1;
    }

    if (workerCount == 0) {
        workerCount = 1;
    }

    for (int worker = 0; worker < workerCount; worker++) {

        // here we normally create a pipe 
        // will it couse waiting ? becuse its one shared pipe ?, which will make the process
        // go sequentially ? 

        // maybe we can make an array of pipes for each child ? 

        int resultPipe[2];

        if (pipe(resultPipe) == -1) {
            perror("pipe");
            freeAllocatedMemory(&matrixC);
            return createMatrix(0, 0);
        }

        pid_t pid = fork();

        if (pid == 0) {
            
            // calculate the range of work. based on the worker id/ number 
            // decided by the for loop above 

            int firstTask = worker * tasksPerWorker;
            int lastTask = firstTask + tasksPerWorker;

            if (lastTask > totalTasks) {
                lastTask = totalTasks;
            }
            
            // here we store the working range for each child in a struct to send it to parent

            struct TransposeChunk chunk = {
                .start = firstTask,
                .end = lastTask
            };

            // The child calculates where each value belongs in the transposed matrix.

            for (int inputIndex = firstTask; inputIndex < lastTask; inputIndex++) {
                int row = inputIndex / matrix.columns;
                int column = inputIndex % matrix.columns;
                int chunkIndex = inputIndex - firstTask;

                chunk.outputIndex[chunkIndex] = column * matrixC.columns + row;
                chunk.values[chunkIndex] = matrix.arr[inputIndex];
            }

            // sned the struct that stores the result 
            close(resultPipe[0]);
            write(resultPipe[1], &chunk, sizeof chunk);
            close(resultPipe[1]);
            _exit(0);

        }

        close(resultPipe[1]);

        if (pid < 0) {
            perror("fork");
            close(resultPipe[0]);
            continue;
        }

        // The parent only joins the already-transposed values from each child.
        struct TransposeChunk chunk;
        if (read(resultPipe[0], &chunk, sizeof chunk) == sizeof chunk) {
            for (int chunkIndex = 0; chunkIndex < chunk.end - chunk.start; chunkIndex++) {

                matrixC.arr[chunk.outputIndex[chunkIndex]] = chunk.values[chunkIndex];
            }
        }

        close(resultPipe[0]);
        waitpid(pid, NULL, 0);
    }

    return matrixC;

}

double matrixAverage(struct Matrix matrix){

    


    int totalTasks = matrix.rows * matrix.columns;
    int workerCount = totalTasks / tasksPerWorker;

    if (totalTasks % tasksPerWorker != 0) {
        workerCount = workerCount + 1;
    }

    if (workerCount == 0) {
        workerCount = 1;
    }

    int resultPipes[workerCount][2];
    int pipeLastProcess[2];

    pid_t workerPids[workerCount];

    for (int worker = 0; worker < workerCount; worker++) {
        if (pipe(resultPipes[worker]) == -1) {
            perror("pipe");
            return 0.0;
        }

        
        
        pipe(pipeLastProcess);
        

        pid_t pid = fork();

        if (pid == 0) {

            int firstTask = worker * tasksPerWorker;
            int lastTask = firstTask + tasksPerWorker;

            if (lastTask > totalTasks) {
                lastTask = totalTasks;
            }

            double avg = 0.0;
            int sum = 0;
            int numberOfTasksForTheLastProcess;
            for (int i = firstTask; i < lastTask; i++) {

                

                sum = sum + matrix.arr[i];

                

                if ((lastTask - firstTask) - 100 == 0){
                    //avg = (sum) / tasksPerWorker;
                }else {
                    numberOfTasksForTheLastProcess = ((lastTask - firstTask) - 100) * -1;
                    
                }

               

            }

            if(worker == workerCount - 1){
                avg = (double)sum  / numberOfTasksForTheLastProcess;
                close(pipeLastProcess[0]);
                write(pipeLastProcess[1],&numberOfTasksForTheLastProcess,sizeof (numberOfTasksForTheLastProcess));
                close(pipeLastProcess[1]);
            }else{
                avg = (double)sum / tasksPerWorker;
            }

            close(resultPipes[worker][0]);
            write(resultPipes[worker][1], &avg, sizeof avg);
            close(resultPipes[worker][1]);

            fflush(stdout);
            _exit(0);
        }

        workerPids[worker] = pid;
        close(resultPipes[worker][1]);

    }

    double totalAvg = 0.0;
    int numberOfTasksForTheLastProcess;

    for (int worker = 0; worker < workerCount; worker++) {

        if (worker == workerCount -1 ){
            double avg = 0.0;
            close(resultPipes[worker][1]);
            read(resultPipes[worker][0],&avg,sizeof avg);
            close(resultPipes[worker][0]);

            
            close(pipeLastProcess[1]);
            read(pipeLastProcess[0],&numberOfTasksForTheLastProcess,sizeof (numberOfTasksForTheLastProcess));
            close(pipeLastProcess[0]);

            totalAvg = totalAvg + (numberOfTasksForTheLastProcess * avg);

        }else{
            double avg = 0.0;
            close(resultPipes[worker][1]);
            read(resultPipes[worker][0],&avg,sizeof avg);
            close(resultPipes[worker][0]);
            totalAvg = totalAvg + (tasksPerWorker * avg);
        }
        
    }

    totalAvg = totalAvg / (((workerCount - 1) * tasksPerWorker) + numberOfTasksForTheLastProcess);


    //avg = (double)sum / (matrix.columns * matrix.rows);
    return totalAvg;

}


int main(){

    srand(time(NULL));
    struct timeval start, end; 
    struct Matrix multiplicationResult;
    struct Matrix transposeResult;
    
    

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

    int multiplicationPipe[2];
    pipe(multiplicationPipe);

    //printMatrix(matrixA);
    int pid1 = fork();

    if( pid1 == 0 ){

        struct timeval startM, endM; 

        printf("============================\n");
        printf("   Matrix Multipication     \n");
        //printf("============================\n");

        gettimeofday(&startM, NULL); 
        struct Matrix multipliedMatrix = matrixMultipication(matrixA, matrixB);
        gettimeofday(&endM, NULL); 

          close(multiplicationPipe[0]);
          write(multiplicationPipe[1], &multipliedMatrix.rows, sizeof(int));
          write(multiplicationPipe[1], &multipliedMatrix.columns, sizeof(int));
          write(multiplicationPipe[1], multipliedMatrix.arr,(size_t)multipliedMatrix.rows * multipliedMatrix.columns * sizeof(int));
          close(multiplicationPipe[1]);
          freeAllocatedMemory(&multipliedMatrix);

        double timeTakenM =(endM.tv_sec - startM.tv_sec) +(endM.tv_usec - startM.tv_usec) / 1000000.0;
        
        printf("\n");

        printf("> Time taken to finish multipcation > %f\n",timeTakenM);

        printf("\n");

        exit(0);

    }else{
        
        int transpositionPipe[2];
        pipe(transpositionPipe);

        int pid2 = fork();

        if( pid2 == 0 ){

            close(multiplicationPipe[0]);
            close(multiplicationPipe[1]);

            struct timeval startT, endT; 

            printf("============================\n");
            printf("   Matrix Transposition     \n");
            //printf("============================\n");

            gettimeofday(&startT, NULL); 
            struct Matrix transposedMtrix = matrixTransposition(matrixA);
            gettimeofday(&endT, NULL);
            
            double timeTakenT =(endT.tv_sec - startT.tv_sec) +(endT.tv_usec - startT.tv_usec) / 1000000.0;
            
            close(transpositionPipe[0]);
            write(transpositionPipe[1], &transposedMtrix.rows, sizeof(int));
            write(transpositionPipe[1], &transposedMtrix.columns, sizeof(int));
            write(transpositionPipe[1], transposedMtrix.arr,(size_t)transposedMtrix.rows * transposedMtrix.columns * sizeof(int));
            close(transpositionPipe[1]);


            //printMatrix(transposedMtrix);

            printf("\n");

            printf("> Time taken to finish Transposition > %f\n",timeTakenT);

            printf("\n");

            //freeAllocatedMemory(&matrixTranspositionResult);
            freeAllocatedMemory(&transposedMtrix);
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


            close(multiplicationPipe[1]);
            read(multiplicationPipe[0], &multiplicationResult.rows, sizeof(int));
            read(multiplicationPipe[0], &multiplicationResult.columns, sizeof(int));
            multiplicationResult = createMatrix(multiplicationResult.rows,multiplicationResult.columns);
            read(multiplicationPipe[0], multiplicationResult.arr,(size_t)multiplicationResult.rows * multiplicationResult.columns * sizeof(int));
            close(multiplicationPipe[0]);

            printMatrix(multiplicationResult);
            freeAllocatedMemory(&multiplicationResult);      


            /// NOTE: becuse the child process that we made to do the transposition operation 
            // have a different memory, when we send the whole struct transposed result matrix
            // the arr will still have the pointer that point to address at the chile memory and not the parent
            // therefore we will ssend the data seperetly then combine them 

            //  close(transpositionPipe[1]);
            // read(transpositionPipe[0],&transposeResult,sizeof(struct Matrix)); 
            // this is wrong 
            // close(transpositionPipe[0]); 

            //printMatrix(transposeResult);
            close(transpositionPipe[1]);
            read(transpositionPipe[0], &transposeResult.rows, sizeof(int));
            read(transpositionPipe[0], &transposeResult.columns, sizeof(int));
            transposeResult = createMatrix(transposeResult.rows, transposeResult.columns);
            read(transpositionPipe[0], transposeResult.arr,(size_t)transposeResult.rows * transposeResult.columns * sizeof(int));
            close(transpositionPipe[0]);

            printMatrix(transposeResult);
            freeAllocatedMemory(&transposeResult);

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