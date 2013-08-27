#include "ga.h" 
#include "math.h" 
#include "mpi.h" 
#include "macdecls.h" 
#define N 1000 
#define pi acos(0)*2 
int main(int argc, char** argv) 
{ 
    int nprocs,myid,nprocssq; 
    int dims[2],chunk[2]; 
    int i,j,k; 
    int stack = 100000, heap = 100000; 
    MPI_Init(&argc,&argv); 
    GA_Initialize(); 
    MA_init(C_DBL,stack,heap); 
    nprocssq = GA_Nnodes(); 
    nprocs = sqrt(nprocssq); 
    myid = GA_Nodeid(); 
    dims[0] = N; dims[1] = N; 
    chunk[0] = N/nprocs; 
    chunk[1] = N/nprocs; 
    int g_a = NGA_Create(C_DBL,2,dims,"Array A",chunk); 
    int lo[2],hi[2]; 
    NGA_Distribution(g_a,myid,lo,hi); 
    int ld[1] = {N/nprocs}; 
    void *ptr; 
    double *local; 
    printf("Myid = %d, lo = [%d,%d] , hi = [%d,%d] , ld = %d \n",myid,lo[0],lo[1],hi[0],hi[1],ld[0]); 
    NGA_Access(g_a,lo,hi,&ptr,ld); 
    local = (double*) ptr; 
    printf("Myid = %d , local[0][0] = %f\n",*local); 
    GA_Sync(); 
    GA_Destroy(g_a); 
    GA_Terminate(); 
    MPI_Finalize(); 
    return 0; 
} 
