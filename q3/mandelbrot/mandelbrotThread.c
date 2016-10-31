#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define RESULT_TAG 0
#define WORK_TAG 1
#define STOP_TAG 2

extern void mandelbrotProcessRowHelper(
        float x0, float y0, float x1, float y1,
        int width, int height, int row,
        int maxIterations,
        int output[]);

//
// workerStatic --
//
// Thread entrypoint.
void workerStatic(int rank, float x0, float y0, float x1, float y1, int width, int height, int maxIterations, int startRow, int nRows) {
    unsigned int i = 0, j = 0;

    int *output = (int*)malloc(sizeof(int) * (width+1));
    for (i = startRow, j = 0; i < height && j < nRows; i++, j++) {
        output[0] = i;
        mandelbrotProcessRowHelper(x0, y0, x1, y1, width, height, i, maxIterations, output+1);
        MPI_Send(output, width+1, MPI_INT, 0, RESULT_TAG, MPI_COMM_WORLD);
    }

    free(output);
}

//
// masterStatic --
//
// Thread entrypoint.
void masterStatic(float x0, float y0, float x1, float y1, int width, int height, int maxIterations, int output[]) {
    int* tmp = (int*)malloc(sizeof(int)*(width+1));
    
    MPI_Status status;

    int rowCount = 0;
    int index = 0;
    while (rowCount < height) {
        MPI_Recv(tmp, width+1, MPI_INT, MPI_ANY_SOURCE, RESULT_TAG, MPI_COMM_WORLD, &status);

        index = tmp[0]*width;
        memcpy((void*)(output+index), tmp+1, sizeof(int)*width);

        rowCount++;
    }

    free(tmp);
}
