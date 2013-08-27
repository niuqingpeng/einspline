#include "ga.h"
#include "gpc.h"
#include "mpi.h"
#define N 4

int me, nprocs;
int g_test;
int hsend = 0;
int hrecv = 0;
int gpc_send_handler(int to, int from, void *hdr,   int hlen,
        void *data,  int dlen,
        void *rhdr,  int rhlen, int *rhsize,
        void *rdata, int rdlen, int *rdsize,
        int rtype) {
    int num = dlen / sizeof(int);
    int lo[2];
    int hi[2];
    int array[1];
    int ld[2 - 1];

    lo[0] = 0;
    lo[1] = 0;
    hi[0] = 0;
    hi[1] = 0;
    array[0] = me + 700;
    ld[0] = 1;

    NGA_Put(g_test, lo, hi, array, ld);
    return GPC_DONE;
}

void send_gpc() {
    gpc_hdl_t nbh;
    ARMCI_Gpc_init_handle(&nbh);
    if(me==0)
    {
        int p = 2;
        int hdr[1];
        int data[2] = {123, 126};
        char rhdr[1];
        int rdata[2];
        if(ARMCI_Gpc_exec(hsend, p, &hdr, sizeof(hdr), data, sizeof(data), rhdr, sizeof(rhdr),
                    rdata, sizeof(rdata), NULL/*&nbh*/)) {
            fprintf(stderr,"ARMCI_Gpc_exec failed\n");
        }
    }
}

int main(int argc, char**argv) {
    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&me);
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
    //ARMCI_Init_args(&argc, &argv);
    GA_Initialize();
    /*g_test = GA_Create_handle();
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
    
    hsend = ARMCI_Gpc_register(gpc_send_handler);
    if(me==0) {
        send_gpc();
    }
    
    ARMCI_Barrier();
    
    
    GA_Print(g_test);
    GA_Destroy(g_test);
    GA_Terminate();*/
    MPI_Finalize();
    return 0;
}
