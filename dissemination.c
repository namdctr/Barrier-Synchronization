#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include <stdint.h>
#define MAXTHREADS 10000

int NUM_THREADS, NUM_BARRIERS;

// Structure is defined to hold the flags for each thread and its partner threads.
typedef struct flags {
	int myflags[2][MAXTHREADS];				
	int *partnerflags[2][MAXTHREADS];	
} flags;

void dissemination_barrier( flags *localflags, int *sense, int *parity, int *proc) {
	/*
		localflags: Local flags for the calling thread.
		sense: Current synchronization sense.
		parity: Current parity bit.
		proc: Number of processors (log2 of the number of threads)
	*/

	int p = *parity;
	int i;
	for(i = 0; i < *proc; i++) {
		#pragma omp critical 
		{
			*localflags->partnerflags[p][i] = *sense;
		}
		while(localflags->myflags[p][i] != *sense){}
	}

	if(*parity == 1)
		*sense = !*sense;
	*parity = 1 - *parity;
}


int main(int argc, char **argv) {

	// check syntax
	if (argc==3) {
		NUM_THREADS = atoi(argv[1]);		// ASCII to integer
		NUM_BARRIERS = atoi(argv[2]);		// ASCII to integer
	} else {
        printf("Syntax: ./dissemination num_threads num_barriers\n");
        exit(-1);
    }

	omp_set_num_threads(NUM_THREADS);
	double startTime[NUM_BARRIERS][NUM_THREADS];
	double endTime[NUM_BARRIERS][NUM_THREADS];

	flags allnodes[NUM_THREADS]; 				// array of flags, each is a flag for a thread
	int proc = ceil(log(NUM_THREADS)/log(2));	// number of rounds

	#pragma omp parallel shared(allnodes, proc) 
	{
		int thread_num = omp_get_thread_num();
		int num_threads = omp_get_num_threads();

		int i, j, k, l, m;
		int parity = 0; 	//processor private
		int sense = 1; 		//processor private
		flags *localflags = &allnodes[thread_num]; //processor private
		int temp, y;

		#pragma omp critical
		// initilize local flags for each thread to 0
		for (l = 0; l < NUM_THREADS; l++)
			for (m = 0; m < 2; m++)
				for (k = 0; k < proc; k++)
					allnodes[l].myflags[m][k] = 0;

		for (i = 0; i < NUM_BARRIERS; i++) {
			for (j = 0; j < 5000; j++);
			printf("Thread %d of %d entering barrier %d.\n", thread_num, num_threads, i+1);

			// initialize partner flags
			#pragma omp critical
			for (j = 0; j < NUM_THREADS; j++) {
				for (k = 0; k < proc; k++) {
					temp = ceil(pow(2,k));
					if (j == (thread_num + temp) % NUM_THREADS) {
						allnodes[thread_num].partnerflags[0][k] =  &allnodes[j].myflags[0][k];
						allnodes[thread_num].partnerflags[1][k] =  &allnodes[j].myflags[1][k];
					}
				}
			}

			startTime[i][thread_num] = omp_get_wtime();
			dissemination_barrier(localflags, &sense, &parity, &proc);
			endTime[i][thread_num] = omp_get_wtime();
			printf("Thread %d of %d exiting barrier %d.\n", thread_num, num_threads, i+1);
		}
	}

	// Time taken of each thread at each barrier
	int i = 0, j;
	while(i < NUM_BARRIERS) { 
		j = 0;
		printf("At Barrier #%d:\n", i+1);
		while(j < NUM_THREADS) {
			printf("Time taken by Thread #%d to complete = %f\n", j+1, endTime[i][j]-startTime[i][j]);
			j++;
		}
		i++;
	}
	printf("\n\n");

	// Avg time taken of each thread
	i = 0;
	while(i < NUM_THREADS) {
		j = 0;
		float sum = 0.0;
		while(j < NUM_BARRIERS) {
			sum += endTime[j][i]-startTime[j][i];
			j++;
		}
		printf("Average time taken by Thread #%d to complete = %f \n", i+1, sum/(float)NUM_BARRIERS);
		i++;
	}
	printf("\n\n");

	// Avg time taken by all threads
	i = 0;
	float netVal = 0.0;
	float sum = 0.0;
	while(i < NUM_THREADS) {
		j = 0, sum = 0.0;
		while(j < NUM_BARRIERS)
		{
			sum+=endTime[j][i]-startTime[j][i];
			j++;
		}
		netVal += sum/(float)NUM_BARRIERS;
		i++;
	}
	printf("ALGORITHM -- DISSEMINATION (Thread# = %d, Barrier# = %d) -- Average time taken by threads to complete = %f s\n\n", NUM_THREADS, NUM_BARRIERS, netVal/NUM_THREADS);
	return 0;
}
