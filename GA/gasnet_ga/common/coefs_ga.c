#include "coefs_ga.h"
#include <mpi.h>
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

long mini_cube_sum(void* mini_cube, int nsplines) {
  int N = nsplines;
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
  /*if(GA_Nodeid()==2) {
          printf("coefs=\n");
  for(i=0;i<4;i++)
    for(j=0;j<4;j++,printf("\n"))
      for (k=0; k<4; k++) {
          int* coefs = coefs1 + ((i)*xs + (j)*ys + (k)*zs);
          n=0;
          //for (n=0; n<nsplines; n++) {
              printf("%d\t", coefs[n]);
          //}
      }
  }*/
  return sum;
}

void print_range(char *pre,int ndim,
                            int lo[], int hi[], char* post)
{
        int i;

        printf("%s[",pre);
        for(i=0;i<ndim;i++){
                printf("%d:%d",lo[i],hi[i]);
                if(i==ndim-1)printf("] %s",post);
                else printf(",");
        }
}

int rand_index(int low,int high)
{
    int ret=rand()%(high-low+1)+low;
    return ret;
}

double timing(int isBegin, const char msg[])
{
    int me;
    MPI_Comm_rank(MPI_COMM_WORLD,&me);
    static double beginTime=0.1;
    if(isBegin)
    {
        beginTime=MPI_Wtime()*1000;
        printf("%s pid%d begin at %lf\n",msg, me,beginTime);
        return beginTime;
    }
    else
    {
        double endTime=MPI_Wtime()*1000;
        printf("%s pid%d end at %lf with period %lf\n", msg, me,endTime,endTime-beginTime);
        return endTime;
    }
}

//input: ga_coefs x y z
long coefs_ghost_access_3d(int g_a, int x, int y, int z, int nsplines)
{
  void *mini_cube;
  int lo[4],hi[4],ld[4], dims_with_ghosts[4];
  NGA_Distribution(g_a, GA_Nodeid(), lo, hi);
  NGA_Access_ghosts(g_a, dims_with_ghosts, &mini_cube, ld);
  /*if(GA_Nodeid()==0) {
      printf("\nlo=");
      printf("%d %d %d %d", lo[0], lo[1], lo[2], lo[3]);
      printf("\nhi=");
      printf("%d %d %d %d", hi[0], hi[1], hi[2], hi[3]);
      printf("\nld=");
      int i=0;
      for (i=0; i<3; i++) {
          printf("%d ", ld[i]);
      }
      printf("\ndims_with_ghosts=");
      for (i=0; i<4; i++) {
          printf("%d ", dims_with_ghosts[i]);
      }
      printf("\n");
  }*/
  int* coefs1=(int*)mini_cube;
  assert(ld[2]==nsplines);
  intptr_t zs = ld[2];
  intptr_t ys = ld[1] * zs;
  intptr_t xs = ld[0] * ys;
  coefs1 = coefs1 + 3*xs + 3*ys + 3*zs;
  int i, j, k, n;
  //for (i=0; i<4; i++)
    //for (j=0; j<4; j++)
  assert(lo[0]<=x && x<=hi[0]);
  assert(lo[1]<=y && y<=hi[1]);
  if(lo[2]<=z && z<=hi[2]) ;
  else printf("%d: lo[2]=%d hi[2]=%d z=%d\n", GA_Nodeid(), lo[2], hi[2], z);
  assert(lo[2]<=z && z<=hi[2]);
  long sum=0;
  for (i=0; i<4; i++)
    for (j=0; j<4; j++)
      for (k=0; k<4; k++) {
          int* coefs = coefs1 + ((x-lo[0]+i)*xs + (y-lo[0]+j)*ys + (z-lo[2]+k)*zs);
          for (n=0; n<nsplines; n++) {
              sum += coefs[n];
          }
      }
/*  if(GA_Nodeid()==2) {
          printf("ghost coefs=\n");
  for(i=0;i<4;i++)
    for(j=0;j<4;j++,printf("\n"))
      for (k=0; k<4; k++) {
          int* coefs = coefs1 + ((x-lo[0]+i)*xs + (y-lo[0]+j)*ys + (z-lo[2]+k)*zs);
          n=0;
  //        for (n=0; n<nsplines; n++) {
              printf("%d\t", coefs[n]);
    //      }
      }
  }*/

//  double begin=MPI_Wtime()*1000;
//  double end=MPI_Wtime()*1000;
//  ga_coefs->amount++;
//  ga_coefs->sumt+=end-begin;
//  printf("haah %ld\n", sum);
  return sum;
}

int coefs_ga_create_ghost(int Nx, int Ny, int Nz, int nsplines) {
  int data[Nx*Ny*Nz];
  int g_a,dims[4]={Nx,Ny,Nz,nsplines},chunk[4]={-1,-1,-1,nsplines};
  int width[4] = {3, 3, 3, 0};
  int type=C_INT;
  //g_a=NGA_Create(type,4,dims,"Coefs",chunk);
  g_a=NGA_Create_ghosts(type, 4, dims, width, "Coefs", chunk);
  int lo[4],hi[4],ld[3];
  //int value=9; GA_Fill(g_a,&value);
  GA_Print_distribution(g_a);
  return g_a;
}

