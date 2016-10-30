#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_LEN 1<<20

/***************************************************************************************************
 *
 * Input - 
 *          arg 1 = Number of bytes to use in communication
 *          arg 2 = Number of repetitions for ping-pong messages
 *          arg 3 = Number of repititions for computation/Loop overhead/Timer overhead calculation
 *
 * Output - csv data in following sequence
 *          1. Number of bytes
 *          2. Avergage communication time (Micro Sec)
 *          3. Approx bandwidth (MB/s)
 *          4. Computation time (Micro seconds computation)
 *          5. Clock Resolution 
 *          6. Average Timer overhead (Micro sec)
 *          7. Empty Loop overhead per iteration (Micro seconds per iter)
 *          
 * **************************************************************************************************/        
int main(int argc, char** argv) {
    int numBytes = atoi(argv[1]);
    if (numBytes > MAX_LEN) {
        printf("\nMax number of bytes = %d", MAX_LEN);
        return 1;
    }

    int reps = atoi(argv[2]);
    int compReps = atoi(argv[3]);

    int dest = 0; 
    int source = 1; 
    int repIdx = 0;
    int rank = 0;
    int comm_size = 0;
    int tag = 1;
    int retVal = 0;

    char msg[MAX_LEN];
    int i = 0;
    for (i = 0; i < MAX_LEN; ++i) {
        msg[i] = 'a';
    }
    
    double startTime = 0.0; 
    double endTime = 0.0; 
    double deltaT = 0.0;
    double avgT = 0.0;
    double bw = 0.0;
    double clockResolution = 0.0;
    double avgTimerOvhd = 0.0;
    double compT = 0.0;
    double emptyLoopT = 0.0;

    float* x = (float*)malloc(sizeof(float) * compReps);;
    float* y = (float*)malloc(sizeof(float) * compReps);;
    float* z = (float*)malloc(sizeof(float) * compReps);;
    for (i = 0; i < compReps; ++i) {
        x[i] = 73215.77987;
        y[i] = 73215.77987;
        z[i] = 73215.77987;
    }

    MPI_Status status;

    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

    // Get the rank of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0 && comm_size != 2) {
        printf("\nNumber of processes should be 2");
        MPI_Abort(MPI_COMM_WORLD, retVal);
        return 1;
    }
    

    // Step 1: Calculate Timer overhead
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) {
        for (repIdx = 0; repIdx < compReps; ++repIdx) {
            startTime = MPI_Wtime();
            endTime = MPI_Wtime();
            avgTimerOvhd += (endTime - startTime);
        }
        avgTimerOvhd = ((double)avgTimerOvhd*1000000)/compReps; // Micro seconds
    }

    // Step 2: Calculate Loop overhead
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) {
        startTime = MPI_Wtime();
        for (repIdx = 0; repIdx < compReps; ++repIdx) {
            asm("");
        }
        endTime = MPI_Wtime();
        emptyLoopT=((double) (endTime - startTime)*1000000) / compReps; // Micro Sec per iteration
    }
    
    // Step 2: Calculate Computation time
    MPI_Barrier(MPI_COMM_WORLD);
    
    if (rank == 0) {
        startTime = MPI_Wtime();
        for (repIdx = 0; repIdx < compReps; ++repIdx) {
            z[repIdx] += x[repIdx] * y[repIdx];
        }
        endTime = MPI_Wtime();    
        compT = ((double)(endTime - startTime)*1000000 - avgTimerOvhd - emptyLoopT*compReps) / compReps; // Micro seconds
    }

    // Step 3: Calculate Communication time, using ping-pong messages
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        dest = 1;
        source = 1;

        startTime = MPI_Wtime();

        for (repIdx = 0; repIdx < reps; ++repIdx) {    
            retVal = MPI_Send(msg, numBytes, MPI_BYTE, dest, tag, MPI_COMM_WORLD);
            if (retVal != MPI_SUCCESS) {
                printf("\n Sending Error!!");
                MPI_Abort(MPI_COMM_WORLD, retVal);
                return 1;
            }

            retVal = MPI_Recv(msg, numBytes, MPI_BYTE, source, tag, MPI_COMM_WORLD, &status);
            if (retVal != MPI_SUCCESS) {
                printf("\n Receiveing Error!!");
                MPI_Abort(MPI_COMM_WORLD, retVal);
                return 1;
            }
        }

        endTime = MPI_Wtime();
        deltaT = endTime - startTime;
        clockResolution = MPI_Wtick();
        avgT = (deltaT * 1000000 - emptyLoopT*reps - avgTimerOvhd) / (2*reps); // Micro seconds
        bw = (((double)2*numBytes*reps) / ((deltaT*1000000 - emptyLoopT*reps - avgTimerOvhd) / 1000000)); // Bytes/sec
        
        printf("\n%d,%8.8f,%8.8f,%8.8f,%8.8f,%8.8f,%8.8f\n",numBytes, avgT, (bw / 1000000), compT, clockResolution, avgTimerOvhd, emptyLoopT);
    } else if (rank == 1) {
        dest = 0;
        source = 0;

        for (repIdx = 0; repIdx < reps; ++repIdx) {
            retVal = MPI_Recv(msg, numBytes, MPI_BYTE, source, tag, MPI_COMM_WORLD, &status);
            if (retVal != MPI_SUCCESS) {
                printf("\n Receiveing Error!!");
                MPI_Abort(MPI_COMM_WORLD, retVal);
                return 1;
            }

            retVal = MPI_Send(msg, numBytes, MPI_BYTE, dest, tag, MPI_COMM_WORLD);
            if (retVal != MPI_SUCCESS) {
                printf("\n Sending Error!!");
                MPI_Abort(MPI_COMM_WORLD, retVal);
                return 1;
            }
        }
    }

    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}

