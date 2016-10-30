#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    int rank = 0;
    int comm_size = 0;
    int tag = 0;
    int retVal = 0;

    double startTime = 0.0; 
    double endTime = 0.0; 
    double deltaT = 0.0;
    double minT = 1;
    double maxT = 0.0;
    double avgT = 0.0;
    double tmpT = 0;
    
    MPI_Status status;

    int idx = 0;

    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    // Get the rank of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    startTime = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    endTime = MPI_Wtime();

    deltaT = endTime - startTime;

    if (rank == 0) {
        minT = deltaT;
        maxT = deltaT;
        avgT = deltaT;
        for (idx = 1; idx < comm_size; ++idx) {
            retVal = MPI_Recv(&tmpT, 1, MPI_DOUBLE, idx, tag, MPI_COMM_WORLD, &status);
            if (retVal != MPI_SUCCESS) {
                printf("Recv error from process %d", idx);
                MPI_Abort(MPI_COMM_WORLD, retVal);
                return 1;;
            }

            if (tmpT < minT) {
                minT = tmpT;
            }

            if (tmpT > maxT) {
                maxT = tmpT;
            }

            avgT += tmpT;
        }

        // Micro seconds
        minT *= 1000000;
        maxT *= 1000000;
        avgT *= 1000000;

        avgT = (avgT / comm_size);
        printf("\n%8.8f,%8.8f,%8.8f", minT, maxT, avgT);
    } else {
        retVal = MPI_Send(&deltaT, 1, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD);
    }

    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}

