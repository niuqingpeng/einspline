#this ga-5-0 svn version works for gpc code
#GA_HOME=/home/arafatm/GA/ga_svn/ga-5-0/build

#GA_HOME=/home/arafatm/GA/ga-5-1-1/build
#GA_HOME=/home/niuq/GA/ga-5-1/slurm-gpc-build
GA_HOME=/home/niuq/GA/ga-5-1/mpi-slurm-gpc-build

ARMCI_LIBS=$(GA_HOME)/lib
ARMCI_INCLUDE=$(GA_HOME)/armci/src
GA_FFLAGS=-i8
GA_LIBS=$(GA_HOME)/lib -lga -larmci  -libverbs 
GA_INCLUDE=$(GA_HOME)/include

run: main.o
	mpicc  -o run main.o -L$(GA_LIBS)
main.o: main.c
	mpicc -I$(GA_INCLUDE) -c -o main.o main.c
clean:
	rm *.o run
