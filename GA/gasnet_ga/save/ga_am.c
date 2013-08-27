#include <mpi.h>
#include <unistd.h>
#include <ga.h>
#include <gasnet.h>
#include <test.h>
#include "macdecls.h"
typedef struct {
  int Mx,My,Mz;//data dimensions
  int nsplines;
  int g_a;

  double sumt;
  int amount;
}ga_coefs_t;
void coefs_ga_get_3d(ga_coefs_t *ga_coefs,void *mini_cube,int x,int y,int z)
{
  int nsplines = ga_coefs->nsplines;
  int lo[4],hi[4],ld[4];
  lo[0]=x;lo[1]=y;lo[2]=z;lo[3]=0;
  hi[0]=x+3;hi[1]=y+3;hi[2]=z+3;hi[3]=nsplines-1;
  ld[0]=ld[1]=4;ld[2]=nsplines;
  double begin=MPI_Wtime()*1000;
  NGA_Get(ga_coefs->g_a,lo,hi,mini_cube,ld);
  //ld {4 4 32}
  double end=MPI_Wtime()*1000;
  ga_coefs->amount++;
  ga_coefs->sumt+=end-begin;
  return;
}

int mynode = 0;
int numnode = 0;

#define hidx_ping_shorthandler   201
#define hidx_pong_shorthandler   202
#define hidx_done_shorthandler   213
volatile int flag = 0;
volatile int reply_node = -1;
void ping_shorthandler(gasnet_token_t token, gasnet_handlerarg_t ix,
        gasnet_handlerarg_t iy, gasnet_handlerarg_t iz) {
  int sum=ix+iy+iz+mynode*100;
  GASNET_Safe(gasnet_AMReplyShort1(token, hidx_pong_shorthandler, sum));
}

void pong_shorthandler(gasnet_token_t token, gasnet_handlerarg_t sum) {
  reply_node = sum;
  flag=1;
}

volatile int done = 0;
void done_shorthandler(gasnet_token_t token) {
  done = 1;
}

void tests()
{
    flag = -1;
    int ix=0, iy=1, iz=2;
    if(GA_Nodeid()==0)
    {
        GASNET_Safe(gasnet_AMRequestShort3(1, hidx_ping_shorthandler, ix, iy, iz));
        GASNET_BLOCKUNTIL(flag == 1);
    }
    printf("reply=%d\n", reply_node);
}

void coefs_ga_create(ga_coefs_t* ga_coefs, int Nx, int Ny, int Nz) {
  int data[Nx*Ny*Nz];
  int num_splines = 32;
  int g_a,dims[4]={Nx,Ny,Nz,num_splines},chunk[4]={-1,-1,-1,num_splines};
  int width[4] = {3, 3, 3, 0};
  int type=C_INT;
  //g_a=NGA_Create(type,4,dims,"Coefs",chunk);
  g_a=NGA_Create_ghosts(type, 4, dims, width, "Coefs", chunk);
  int lo[4],hi[4],ld[3];
  double value=9.0;
  GA_Fill(g_a,&value);
  GA_Print_distribution(g_a);
}

int isMPIinit;
int main(int argc, char**argv)
{
  gasnet_handlerentry_t htable[] = {
    { hidx_ping_shorthandler,  ping_shorthandler  },
    { hidx_pong_shorthandler,  pong_shorthandler  },
    { hidx_done_shorthandler,  done_shorthandler  }
  };
  GASNET_Safe(gasnet_init(&argc, &argv));
  GASNET_Safe(gasnet_attach(htable, sizeof(htable)/sizeof(gasnet_handlerentry_t),
                            TEST_SEGSZ_REQUEST, TEST_MINHEAPOFFSET));
  gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
  gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);

  if (MPI_Initialized(&isMPIinit) != MPI_SUCCESS) { /* test if MPI already init */
    fprintf(stderr, "Error calling MPI_Initialized()\n");
    abort();
  }
  if (!isMPIinit) MPI_Init(&argc, &argv); /* MPI not init, so do it */
  int nprocs, me;
  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD,&me);

  GA_Initialize(); MSG("GA init done.");
  const int heap=3000000, stack=300000;
  if(! MA_init(C_INT,stack,heap) ) GA_Error((char *) "MA_init failed",stack+heap);
  MSG("MA init done.");
  MPI_Barrier(MPI_COMM_WORLD);
  MSG("barrier done."); fflush(stdout);
  assert(gasnet_mynode()==GA_Nodeid());
  mynode = gasnet_mynode();
  numnode = gasnet_nodes();
  MSG("gasnet node done.");
  int Nx=97; int Ny=97; int Nz = 97;
  Nx+=3; Ny+=3; Nz+=3;
  coefs_ga_create();
  MPI_Barrier(MPI_COMM_WORLD);
  tests(); BARRIER();
  GA_Terminate();
  if(!isMPIinit) MPI_Finalize();
  MSG("mpi finalize done.");
  BARRIER();
  gasnet_exit(0);
}
