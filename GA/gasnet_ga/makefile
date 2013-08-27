GASNET_HOME=/home/niuq/tools/GASNet-1.20.2
GA_HOME=/home/niuq/GA/svn_ga/ga-5-0/slurm-gpc-build
#GA_HOME=/home/niuq/GA/ga-5-1/mpi-slurm-gpc-build
CFLAGS=-I$(GA_HOME)/armci/src -I$(GA_HOME)/armci -g
GA_INCLUDE=${GA_HOME}/include
GA_LIBS=$(GA_HOME)/lib
GA_LDFLAGS=-lga -larmci -lgfortran -lifcore -lifport #-libverbs -lm 
SOURCES=$(wildcard *.c)
EXE=$(SOURCES:%.c=%.x)
OBJS=$(SOURCES:%.c=%.o)
all:${EXE}
%.o:%.c
	mpicc -I${PWD}/common -I$(GA_INCLUDE) $(CFLAGS) -I"${GASNET_HOME}/tests" -DGASNET_SEQ -D_REENTRANT -D_GNU_SOURCE -I${GASNET_HOME} -I${GASNET_HOME}/ibv-conduit -I${GASNET_HOME}/other -DGASNET_CONDUIT_IBV  -I${GASNET_HOME}/other/firehose -I${GASNET_HOME}/vapi-conduit   -I${GASNET_HOME}/extended-ref -I${GASNET_HOME}/ibv-build    -O3 --param max-inline-insns-single=35000 --param inline-unit-growth=10000 --param large-function-growth=200000 -Winline -c -o $@ $^
common/%.o:common/%.c
	mpicc -I${PWD}/common -I$(GA_INCLUDE) $(CFLAGS) -I"${GASNET_HOME}/tests" -DGASNET_SEQ -D_REENTRANT -D_GNU_SOURCE -I${GASNET_HOME} -I${GASNET_HOME}/ibv-conduit -I${GASNET_HOME}/other -DGASNET_CONDUIT_IBV  -I${GASNET_HOME}/other/firehose -I${GASNET_HOME}/vapi-conduit   -I${GASNET_HOME}/extended-ref -I${GASNET_HOME}/ibv-build    -O3 --param max-inline-insns-single=35000 --param inline-unit-growth=10000 --param large-function-growth=200000 -Winline -c -o $@ $^
%.x:%.o common/coefs_ga.o
	mpicc  -O3 --param max-inline-insns-single=35000 --param inline-unit-growth=10000 --param large-function-growth=200000 -Winline -o $@ $^  -L${GASNET_HOME}/ibv-build/ibv-conduit -L/usr/lib64    -lgasnet-ibv-seq -libverbs    -lpthread  -lrt -L/usr/lib/gcc/x86_64-redhat-linux/4.4.7 -lgcc -lm -L$(GA_LIBS) $(GA_LDFLAGS) 
clean:
	rm -rf *.o *.x
