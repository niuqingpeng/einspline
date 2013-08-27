#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "mpi.h"
#include <assert.h>
#include "armci.h"
#include "gpc.h"
#include "ga.h"

#define count 1024 
#define N 4

int me, nprocs;
int g_test;

//extern "C" int testGetPut(double *);

int myarray[count];
int sendToRemoteGPU(void *data,int p,int num){
    MPI_Send(data,num, MPI_INT,p,1,MPI_COMM_WORLD);
    return 0;
}
int receiveFromRemoteGPU(int p,int num){
    MPI_Status status;
    MPI_Recv(myarray,num, MPI_INT,p,1,MPI_COMM_WORLD,&status);
    return 0;
}

int hsend = 0;
int hrecv = 0;
void testMPI(){
    int i,j;
    if(me==0){
        for(i=0;i<count;i++) myarray[i] = 98;
        double starttime, endtime; 
        starttime = MPI_Wtime(); 

        for(j=0;j<100;j++){
            sendToRemoteGPU(myarray, 1, count);
            receiveFromRemoteGPU(1,1);
        }
        endtime   = MPI_Wtime(); 
        printf("MPI took %f mseconds\n",(endtime-starttime)*1000/100); 
    }
    if(me==1){
        for(j=0;j<100;j++){
            receiveFromRemoteGPU(0,count);
            putGPU(myarray,count*sizeof(int));
            myarray[0] = 7;
            sendToRemoteGPU(myarray, 0, 1);
        }
    }

}
int gpc_send_handler(int to, int from, void *hdr,   int hlen,
        void *data,  int dlen,
        void *rhdr,  int rhlen, int *rhsize,
        void *rdata, int rdlen, int *rdsize,
        int rtype){

    int *n = (int *)data;
    int num = dlen/sizeof(int);
    /*     printf("send handler\n");
           printf("me = %d  to %d from %d\n",me, to, from);
           int i;
           for(i=0;i<num;i++){
           printf("%d ",n[i]);
           }
           printf("\n");
           fflush(stdout);
           */    

    //n[0] = 9567;
    putGPU(n, dlen);


  int lo[2];
  int hi[2];
  int array[2];
  int ld[2 - 1];

  lo[0] = 0;
  lo[1] = 0;

  hi[0] = 0;
  hi[1] = 0;

  array[0] = 678;
  array[1] = 908;

  ld[0] = 1;

  NGA_Put(g_test,lo,hi,array,ld);
    //   int copyarray[num];

    //   getGPU(copyarray, dlen);


    //    for (i = 0; i< num; i++)
    //      printf("%d ", copyarray[i]);
    //  printf("\n");


    return GPC_DONE;
}
int gpc_recv_handler(int to, int from, void *hdr,   int hlen,
        void *data,  int dlen,
        void *rhdr,  int rhlen, int *rhsize,
        void *rdata, int rdlen, int *rdsize,
        int rtype){

    //    printf("recv handler\n");
    //    int *n = (int *)data;
    //    printf("me = %d  to %d from %d\n",me, to, from);
    fflush(stdout);
    int num = rdlen/sizeof(int);
    int i;

    int copyarray[count];

    getGPU(copyarray, rdlen);
    //    getGPU(rdata, rdlen);
    //    rdata = copyarray;


    int *p = (int *)rdata;
    for (i = 0; i< num; i++){
        //      printf("%d ", copyarray[i]);
        p[i] = copyarray[i]+1;
    }
    // printf("\n");

    return GPC_DONE;
}
int initGPC(){

    hsend = ARMCI_Gpc_register(gpc_send_handler);
    hrecv = ARMCI_Gpc_register(gpc_recv_handler);

    return 0;
}
int val[count];
void send_gpc(){
    gpc_hdl_t nbh;

    int p = 1;
    char rheader[1];
    int hlen, dlen, rhlen,rhsize,rdsize;
    int header[1];
    hlen=sizeof(int)*1;
    int buffer = count;
    int i;
    //   for(i=0;i<buffer;i++)val[i] = 97;
    int *loc = val;
    dlen = buffer*sizeof(int);
    rhlen = hlen;
    ARMCI_Gpc_init_handle(&nbh);

    if(me==0)
    {
        double starttime, endtime; 
        starttime = MPI_Wtime(); 


        //    for(i=0;i<100;i++)
        if(ARMCI_Gpc_exec(hsend,p, &header, hlen, loc, dlen, rheader, rhlen,
                    loc, sizeof(int), NULL/*&nbh*/))
            fprintf(stderr,"ARMCI_Gpc_exec failed\n");
        /*ARMCI_Gpc_wait(&nbh);*/

        endtime   = MPI_Wtime(); 
        printf("Send took %f mseconds\n",(endtime-starttime)*1000/100); 

    }

    fflush(stdout);
    //ARMCI_AllFence();
    // ARMCI_Barrier();
    //  sleep(5);
}
void recv_gpc(){
    gpc_hdl_t nbh;

    int p = 1;
    int rheader[1];
    int hlen, dlen, rhlen,rhsize,rdsize;
    int header[1];
    hlen=sizeof(int)*1;
    int buffer = count;
    int val[1];
    int i;
    for(i=0;i<1;i++)val[i] = 97;
    int *loc = val;
    dlen = 1*sizeof(int);
    rhlen = hlen;
    ARMCI_Gpc_init_handle(&nbh);

    int rdata[count];
    int rdlen = buffer*sizeof(int);
    if(me==0)
    {
        double starttime, endtime; 
        starttime = MPI_Wtime(); 
        //        for(i=0;i<100;i++)
        if(ARMCI_Gpc_exec(hrecv,p, &header, hlen, loc, dlen, rheader, rhlen,
                    rdata, rdlen, NULL/*&nbh*/))
            fprintf(stderr,"ARMCI_Gpc_exec failed\n");
        /*ARMCI_Gpc_wait(&nbh);*/

        endtime   = MPI_Wtime(); 
        printf("Recieve took %f mseconds\n",(endtime-starttime)*1000/100); 

        for(i=0;i<buffer;i++){
            //    if(rdata[i]!=97) printf("DANGERRRRRRRRRRRRRRR!!!! %d is %d\n",i,rdata[i]);
            printf("%d ",rdata[i]);
        }
        //      printf("\n");
        fflush(stdout);

    }

    //  ARMCI_AllFence();
    //  ARMCI_Barrier();
}

int main(int argc, char**argv){

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&me);
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs);

    //ARMCI_Init_args(&argc, &argv);
    GA_Initialize();
    g_test = GA_Create_handle();
    int *map;
    int ndim = 2;
    int dims[2] = {N, N};
    int ld[1];
    int nblock[2];

    int i,j;

    GA_Set_data(g_test, ndim, dims, MT_C_INT);



    GA_Allocate(g_test);
    GA_Zero(g_test);


    int value = 4;
    GA_Fill(g_test,&value);

    GA_Print(g_test);
    GA_Print_distribution(g_test);




    size_t sizeA = count*sizeof(int);
    cudaInit(sizeA);

    initGPC();

    testMPI();

    if(me==0){
        for(i=0;i<count;i++)val[i] = 97;

        //        for(i=0;i<100;i++){
        send_gpc();
        //      recv_gpc();

        //      }
    }


   // ARMCI_AllFence();
    ARMCI_Barrier();
    
    
    GA_Print(g_test);
    
    
    
    cudaFinalize();

    //  ARMCI_Finalize();
    GA_Destroy(g_test);
    GA_Terminate();
    MPI_Finalize();

    return 0;
}

