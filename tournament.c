#include <omp.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

#define winner 0
#define loser 1
#define bye 2
#define champion 3
#define dropout 4

int NUM_THREADS, NUM_BARRIERS;

struct round_struct {
	int role;
	int vpid;
	int tb_round;
	bool *opponent;
	bool flag;
};
typedef struct round_struct round_struct;
round_struct array[1000][100];

void barrier(int vpid, bool *sense,int rounds) {
	int round = 0;

	while(1) {
		if(array[vpid][round].role == loser){
			*(array[vpid][round]).opponent = *sense;
			while(array[vpid][round].flag != *sense);
			break;
		}

		if(array[vpid][round].role == winner) {
			while(array[vpid][round].flag != *sense);
		}

		if(array[vpid][round].role == champion){
			while(array[vpid][round].flag != *sense);
			*(array[vpid][round]).opponent = *sense;
			break;
		}

		if(round < rounds)
			round = round + 1;
	}

	//wake up
	while(1) {
		if(round > 0)
			round = round - 1;

		if(array[vpid][round].role == winner)
			*(array[vpid][round]).opponent = *sense;

		if(array[vpid][round].role == dropout)
			break;
	}

	*sense = !*sense;
}

int main(int argc, char **argv) {
	bool x = false;

	if (argc==3){
		NUM_THREADS = atoi(argv[1]);
		NUM_BARRIERS = atoi(argv[2]);
	} else {
		printf("Syntax: ./tournament num_threads num_barriers\n");
		exit(-1);
	}

	int counter = 0;
	int rounds = ceil(log(NUM_THREADS)/log(2));

	omp_set_num_threads(NUM_THREADS);
	double startTime[NUM_BARRIERS][NUM_THREADS];
	double endTime[NUM_BARRIERS][NUM_THREADS];
	int i,j,k,l;

	for(j = 0; j < NUM_THREADS; j++) {
		for(k = 0; k <= rounds; k++) {
			array[j][k].flag = false;
			array[j][k].role = -1;
			array[j][k].opponent = &x;
		}
	}

	i = 0;
	int temp = 0, temp2, g = 0, comp, comp_second = 0;

	for(l = 0 ; l < NUM_THREADS; l++) {
		for(k = 0; k <= rounds; k++) {
			temp = k;
			temp2 = l;
			comp = ceil(pow(2,k));
			comp_second = ceil(pow(2,k-1));

			if((k > 0) && (l%comp==0) && ((l + (comp_second))< NUM_THREADS) && (comp < NUM_THREADS)){
				array[l][k].role = winner;
			}

			if((k > 0) && (l%comp == 0) && ((l + comp_second)) >= NUM_THREADS){
				array[l][k].role = bye;
			}

			if((k > 0) && ((l%comp == comp_second))){
				array[l][k].role = loser;
			}

			if((k > 0) && (l==0) && (comp >= NUM_THREADS)){
				array[l][k].role = champion;
			}

			if(k==0) {
				array[l][k].role = dropout;
			}

			if(array[l][k].role == loser) {
				array[l][k].opponent = &array[l-comp_second][k].flag;
			} 

			if(array[l][k].role == winner || array[l][k].role == champion) {
				array[l][k].opponent = &array[l+comp_second][k].flag;
			}
		}
	}

	#pragma omp parallel num_threads(NUM_THREADS) shared(array,counter) 
	{
		int vpid=0;
		bool *sense;
		bool temp = true;

		#pragma omp critical 
		{
			vpid = omp_get_thread_num();
			sense = &temp;
		}

		int num_threads = omp_get_num_threads();
		int i,j;
		int thread_num = omp_get_thread_num();
        
		for(i = 0; i < NUM_BARRIERS; i++){
			for(j = 0; j < 5000; j++);
			printf("Thread %d of %d entering barrier %d.\n", thread_num, num_threads, i+1);
			startTime[i][thread_num] = omp_get_wtime();
			barrier(vpid,sense,rounds);
			endTime[i][thread_num] = omp_get_wtime();
			printf("Thread %d of %d exiting barrier %d.\n", thread_num, num_threads, i+1);
		}
	}

	// Time taken of each thread at each barrier
	i = 0;
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
	printf("ALGORITHM -- TOURNAMENT (Thread# = %d, Barrier# = %d) -- Average time taken by threads to complete = %f s\n\n", NUM_THREADS, NUM_BARRIERS, netVal/NUM_THREADS);
	return 0;
}


