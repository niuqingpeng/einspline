SOURCES=$(wildcard *.c)
EXE=$(SOURCES:%.c=%.x)
OBJS=$(SOURCES:%.c=%.o)
GA_HOME=/home/niuq/GA/svn_ga/ga-5-0/slurm-gpc-build
#GA_HOME=/home/niuq/GA/ga-5-1/gpc-build
#GA_HOME=/home/niuq/tools/ga-5-1/gpc-build
#GA_HOME=/home/niuq/GA/ga-5-1/openmpi-gnu-1.5.5-build
#GA_HOME=/home/arafatm/GA/ga-5-0-2/build-common
CFLAGS=-I$(GA_HOME)/armci/src -I$(GA_HOME)/armci 
GA_INCLUDE=${GA_HOME}/include
GA_LIBS=$(GA_HOME)/lib
GA_LDFLAGS=-lga -larmci -lgfortran -lifcore -lifport #-libverbs -lm 
FFLAGS=-Vaxlib -O3 -w -cm -xW -tpp7 -i8
CC=mpicc
all:$(EXE)
#$(EXE):$(OBJS)
%.x:%.o
	$(CC) $^ -openmp -o $@ -L$(GA_LIBS) $(GA_LDFLAGS) 
%.o:%.c
	$(CC) -I$(GA_INCLUDE) $(CFLAGS) $^  -c -g -o $@
clean:
	rm -rf $(OBJS) $(EXE)

