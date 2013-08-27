#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <mpi.h>
#include <unistd.h>
#include <ga.h>
#include "macdecls.h"
typedef struct {
  int Mx,My,Mz;//data dimensions
  int nsplines;
  int g_a;

  double sumt;
  int amount;
}ga_coefs_t;
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
void coefs_ga_get_3d(ga_coefs_t *ga_coefs,void *mini_cube,int x,int y,int z)
{
  int nsplines = ga_coefs->nsplines;
  int lo[4],hi[4],ld[4];
  lo[0]=x;lo[1]=y;lo[2]=z;lo[3]=0;
  hi[0]=x+3;hi[1]=y+3;hi[2]=z+3;hi[3]=nsplines-1;
  ld[0]=ld[1]=4;ld[2]=nsplines;
  double begin=MPI_Wtime()*1000;
  NGA_Get(ga_coefs->g_a,lo,hi,mini_cube,ld);
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
    int ret=rand()%(high-low+1)+low-3;
    if(ret<0) ret=0;
    return ret;
}
int main(int argc, char**argv)
{
  int nprocs, me;
  int i,j;
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD,&me);
  GA_Initialize();
  int flag=1;
  int Nx=108; int Ny=108; int Nz = 108;
  Nx+=3; Ny+=3; Nz+=3;
  int data[Nx*Ny*Nz];
  int num_splines = 32;
  int g_a,dims[4]={Nx,Ny,Nz,num_splines},chunk[4]={-1,-1,-1,num_splines};
  int type=C_INT;
  g_a=NGA_Create(type,4,dims,"Coefs",chunk);
  int lo[4],hi[4],ld[3];
  double value=9.0;
  GA_Fill(g_a,&value);
  GA_Print_distribution(g_a);
  fflush(stdout);
  if(me==0)
  {
      for (i=0; i<num_splines; i++) 
      {
          for (j=0; j<Nx*Ny*Nz; j++) 
              data[j] = rand()%1000;
          lo[0]=lo[1]=lo[2]=0;
          hi[0]=Nx-1;hi[1]=Ny-1;hi[2]=Nz-1;
          lo[3]=hi[3]=i%num_splines;
          ld[0]=Ny;ld[1]=Nz;ld[2]=1;
          NGA_Put(g_a,lo,hi,data,ld);
      }
  }
  printf("done\n"),fflush(stdout);
  GA_Sync();
  ga_coefs_t *ga_coefs = malloc(sizeof(ga_coefs_t));
  ga_coefs->Mx = Nx; 
  ga_coefs->My = Ny;
  ga_coefs->Mz = Nz;
  ga_coefs->nsplines = num_splines;
  ga_coefs->g_a=g_a;
  int *coefs1 = (int*)malloc((size_t)1*sizeof(int)*4*4*4*num_splines);
  int ix,iy,iz;
  Nx-=3; Ny-=3; Nz-=3;
  ga_coefs->sumt=ga_coefs->amount=0;
  NGA_Distribution(g_a,me,lo,hi);
  int low[16][4],high[16][4];
  for(i=0;i<nprocs;i++)
      NGA_Distribution(g_a,i,low[i],high[i]);
  srand ( time(NULL) );
  int k;
  for(k=0;k<nprocs;k++)
  {
      ga_coefs->sumt=ga_coefs->amount=0;
      {
          for(i=0;i<1000;i++)
          {
              ix=rand_index(low[k][0],high[k][0]);
              iy=rand_index(low[k][1],high[k][1]);
              iz=rand_index(low[k][2],high[k][2]);
              coefs_ga_get_3d(ga_coefs,coefs1,ix,iy,iz);
              mini_cube_sum(coefs1, ga_coefs->nsplines);
          }
      }
      printf("<%d,%d>\t %lf \t %d \t %lf\n", me,k, ga_coefs->sumt, ga_coefs->amount, ga_coefs->sumt/ga_coefs->amount),fflush(stdout);
  }
  free(coefs1);
  GA_Terminate();
  MPI_Finalize();
  return 0;
}

