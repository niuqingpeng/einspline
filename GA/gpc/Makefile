#this ga-5-0 svn version works for gpc code
#GA_HOME=/home/arafatm/GA/ga_svn/ga-5-0/build

#GA_HOME=/home/arafatm/GA/ga-5-1-1/build
GA_HOME=/home/niuq/GA/ga-5-1/slurm-gpc-build

ARMCI_LIBS=$(GA_HOME)/lib
ARMCI_INCLUDE=$(GA_HOME)/armci/src
GA_FFLAGS=-i8
GA_LIBS=$(GA_HOME)/lib -lga -larmci  -libverbs 
GA_INCLUDE=$(GA_HOME)/include

#CUDA_HOME=/usr/local/cuda
CUDA_HOME=/opt/cuda/5.0
#CUDA_LIB=$(CUDA_HOME)/lib64 -lcudart 
CUDA_LIB=$(CUDA_HOME)/lib64 -lcudart
run: dsm.o main.o
	mpicc  -o run dsm.o main.o -L$(GA_LIBS) -L$(CUDA_LIB) 

dsm.o: dsm.cu
	nvcc -arch sm_20 -I$(GA_INCLUDE) -c -o dsm.o dsm.cu
main.o: main.c
	mpicc -I$(CUDA_HOME)/include -I$(GA_INCLUDE) -c -o main.o main.c
clean:
	rm *.o run
