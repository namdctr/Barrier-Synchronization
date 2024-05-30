all: centralized dissemination tournament

centralized: centralized.c
	gcc -fopenmp centralized.c -o centralized

dissemination: dissemination.c
	gcc -fopenmp -lm dissemination.c -o dissemination

tournament: tournament.c
	gcc -fopenmp tournament.c -o tournament
	
clean:
	rm -f *.exe centralized dissemination tournament
