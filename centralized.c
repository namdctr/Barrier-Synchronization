#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int NUM_THREADS, NUM_BARRIERS;

static void centralized_barrier(int *count, int *sense, int *local_sense) {
	int i;
	int n = 0;

	// busy-waiting delay
	for (i = 0; i < 5000; i++){
		n += 1;
	}
	
	// reverse the local sense of the calling thread 
	*local_sense = ! *local_sense; 	 
	
	// create a critical section (block of code that only one thread can execute at a time) to safely decrement count
	// to make sure that this operation is atomic, meaning that it happens as a single, uninterruptible operation
	#pragma omp critical 
	{	
		*count = (*count) - 1;
	}
		
	if (*count == 0) {				
        *count = NUM_THREADS;	
        *sense = *local_sense;  // reverse the sense flag
	} else {
       	while (*sense != *local_sense) {}
	}
}

int main(int argc, char **argv) {
	// checking syntax
  	if (argc == 3) {
		NUM_THREADS = atoi(argv[1]);		// ASCII to integer 
		NUM_BARRIERS = atoi(argv[2]);		// ASCII to integer
  	} else {  
		printf("Syntax: ./centralized num_threads num_barriers\n");
		exit(-1);
	}

	omp_set_num_threads(NUM_THREADS);
	int sense = 1, count = NUM_THREADS;

	double startTime[NUM_BARRIERS][NUM_THREADS];
	double endTime[NUM_BARRIERS][NUM_THREADS];

	#pragma omp parallel shared(sense, count) 
	{
    	int num_threads = omp_get_num_threads(); 
    	int thread_num = omp_get_thread_num();		
    	int local_sense = 1;						
    	long i, j;
		int n = 0;

    	for (i = 0; i < NUM_BARRIERS; i++) {
			for (j = 0; j < 5000; j++);
    		printf("Thread %d of %d entering barrier %d.\n", thread_num, num_threads, i+1);
			startTime[i][thread_num] = omp_get_wtime();
    		centralized_barrier(&count, &sense, &local_sense);	//centralized barrier
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
		printf("Average time taken by Thread #%d to complete = %f ms\n", i+1, sum/(float)NUM_BARRIERS);
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
	printf("ALGORITHM -- SENSE_REVERSE (Thread# = %d, Barrier# = %d) -- Average time taken by threads to complete = %f s\n\n", NUM_THREADS, NUM_BARRIERS, netVal/NUM_THREADS);
	return 0;
}




