#include <mpi.h>
#include <unistd.h>
#include <ga.h>
#include <gasnet.h>
#include <test.h>
#include "macdecls.h"
#include "coefs_ga.h"

int mynode = 0;
int numnode = 0;
int g_a;
int num_splines;

#define hidx_ping_shorthandler   201
#define hidx_pong_shorthandler   202
#define hidx_done_shorthandler   213
volatile int flag = 0;
volatile long reply_node = -1;
void ping_shorthandler(gasnet_token_t token, gasnet_handlerarg_t ix,
        gasnet_handlerarg_t iy, gasnet_handlerarg_t iz) {
  //int sum=ix+iy+iz+mynode*100;
  long sum=coefs_ghost_access_3d(g_a, ix, iy, iz, num_splines);
  //printf("haah2 %ld size long=%d\n", sum, sizeof(long));
  gasnet_handlerarg_t suma = (int)(sum>>32);
  gasnet_handlerarg_t sumb = (int)(sum);

  GASNET_Safe(gasnet_AMReplyShort2(token, hidx_pong_shorthandler, suma, sumb));
}

void pong_shorthandler(gasnet_token_t token, gasnet_handlerarg_t suma, gasnet_handlerarg_t sumb) {
  long sum = (long)suma << 32 | (long)sumb & 0xFFFFFFFFL;
  //printf("haah3 %ld\n", sum);
  reply_node = sum;
  flag=1;
}

volatile int done = 0;
void done_shorthandler(gasnet_token_t token) {
  done = 1;
}

long gasnet_get_minicube_sum(int ix, int iy, int iz) {
    int subscript[] = {ix, iy, iz, 0};
    int pid = NGA_Locate(g_a, subscript);
    GASNET_Safe(gasnet_AMRequestShort3(pid, hidx_ping_shorthandler, ix, iy, iz));
    GASNET_BLOCKUNTIL(flag == 1);
    flag=-1;
    return (long)reply_node;
}

long ga_get_minicube_sum(int x, int y, int z, void* mini_cube)
{
  int nsplines = num_splines;
  int lo[4],hi[4],ld[4];
  lo[0]=x;lo[1]=y;lo[2]=z;lo[3]=0;
  hi[0]=x+3;hi[1]=y+3;hi[2]=z+3;hi[3]=nsplines-1;
  ld[0]=ld[1]=4;ld[2]=nsplines;
  NGA_Get(g_a,lo,hi,mini_cube,ld);
  //ld {4 4 32}
  intptr_t xs = (nsplines<<4);
  intptr_t ys = (nsplines<<2);
  intptr_t zs = nsplines;
  int* coefs1 = (int*) mini_cube;
  long sum = 0;
  int i, j, k, n;
  for (i=0; i<4; i++)
    for (j=0; j<4; j++)
      for (k=0; k<4; k++) {
          int* coefs = coefs1 + ((i)*xs + (j)*ys + (k)*zs);
          for (n=0; n<nsplines; n++) {
              sum += coefs[n];
          }
      }
  return sum;
}

void tests(int Nx, int Ny, int Nz)
{
    flag = -1;
    int repeat=1000;
    int ix[repeat], iy[repeat], iz[repeat];
    int i;
    long gasnet_sum[repeat];
    long ga_sum[repeat];
    for(i=0;i<repeat;i++) {
      ix[i]=rand_index(0, Nx-4);
      iy[i]=rand_index(0, Ny-4);
      iz[i]=rand_index(0, Nz-4);
    }
    int *coefs1 = (int*)malloc((size_t)1*sizeof(int)*4*4*4*num_splines);
    MPI_Barrier(MPI_COMM_WORLD);
    timing(1, "GA begin ");
    for(i=0;i<repeat;i++) {
        ga_sum[i] = ga_get_minicube_sum(ix[i], iy[i], iz[i], coefs1);
    }
    free(coefs1);
    timing(0, "GA end");
    MPI_Barrier(MPI_COMM_WORLD);
    timing(1, "GASNET begin ");
    for(i=0;i<repeat;i++) {
        gasnet_sum[i] = gasnet_get_minicube_sum(ix[i], iy[i], iz[i]);
//        long ga_sum = ga_get_minicube_sum(ix[i], iy[i], iz[i], coefs1);
//        assert(sum==ga_sum);
    }
    timing(0, "GASNET end ");
    BARRIER();
    int equal=0;
    int unequal=0;
    for(i=0;i<repeat;i++) {
        if(gasnet_sum[i]==ga_sum[i]) equal++;
        else unequal++;
    }
    printf("equal=%d unequal=%d\n", equal, unequal);
//        printf("gasnet_sum=%ld ga_sum=%ld\n", sum, ga_sum);
//  GASNET_Safe(gasnet_AMRequestShort3(1, hidx_ping_shorthandler, ix, iy, iz));
//  GASNET_BLOCKUNTIL(flag == 1);
    //printf("reply=%d\n", reply_node);
}

int isMPIinit;
int main(int argc, char**argv)
{
  gasnet_handlerentry_t htable[] = {
    { hidx_ping_shorthandler,  ping_shorthandler  },
    { hidx_pong_shorthandler,  pong_shorthandler  },
    { hidx_done_shorthandler,  done_shorthandler  }
  };
  MSG("before gasnet init"); fflush(stdout);
  GASNET_Safe(gasnet_init(&argc, &argv));
  MSG("gasnet init done."); fflush(stdout);
  GASNET_Safe(gasnet_attach(htable, sizeof(htable)/sizeof(gasnet_handlerentry_t),
                            TEST_SEGSZ_REQUEST, TEST_MINHEAPOFFSET));
  MSG("gasnet attach done."); fflush(stdout);
  gasnet_barrier_notify(0, GASNET_BARRIERFLAG_ANONYMOUS);
  gasnet_barrier_wait(0, GASNET_BARRIERFLAG_ANONYMOUS);
  MSG("gasnet barrier start done."); fflush(stdout);

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
  int Nx=97; int Ny=97; int Nz = 97; int nsplines = 32;
  num_splines = nsplines;
  Nx+=3; Ny+=3; Nz+=3;
  g_a = coefs_ga_create_ghost(Nx, Ny, Nz, nsplines);
  if(me==0)
  {
      int i;
      int data[Nx*Ny*Nz];
      int lo[4],hi[4],ld[3];
      for (i=0; i<nsplines; i++)
      {
          int x, y, z;
          for (x=0; x<Nx; x++)
              for (y=0; y<Ny; y++)
                  for (z=0; z<Nz; z++) {
                      data[x*(Ny*Nz)+y*Nz+z] = (x*100*100+y*100+z)*100+i;
                  }
          lo[0]=lo[1]=lo[2]=0;
          hi[0]=Nx-1;hi[1]=Ny-1;hi[2]=Nz-1;
          lo[3]=hi[3]=i%nsplines;
          ld[0]=Ny;ld[1]=Nz;ld[2]=1;
          NGA_Put(g_a,lo,hi,data,ld);
      }
  }
  GA_Update_ghosts(g_a);
  GA_Sync();
  MPI_Barrier(MPI_COMM_WORLD);
  tests(Nx, Ny, Nz);
  GA_Terminate();
  if(!isMPIinit) MPI_Finalize();
  MSG("mpi finalize done.");
  BARRIER();
  gasnet_exit(0);
}

