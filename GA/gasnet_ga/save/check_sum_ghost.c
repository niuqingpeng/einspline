#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <mpi.h>
#include <unistd.h>
#include <ga.h>
#include <assert.h>
#include "macdecls.h"

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

long coefs_ghost_access_3d(ga_coefs_t *ga_coefs, void *mini_cube, int x, int y, int z)
{
  int nsplines = ga_coefs->nsplines;
  int lo[4],hi[4],ld[4], dims_with_ghosts[4];
  NGA_Distribution(ga_coefs->g_a, GA_Nodeid(), lo, hi);
  NGA_Access_ghosts(ga_coefs->g_a, dims_with_ghosts, &mini_cube, ld);
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
  return sum;
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

int main(int argc, char**argv)
{
  int nprocs, me;
  int i,j;
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD,&me);
  GA_Initialize();
  const int heap=3000000, stack=300000;
  if(! MA_init(C_INT,stack,heap) ) GA_Error((char *) "MA_init failed",stack+heap /*error code*/);
  int flag=1;
  int Nx=97; int Ny=97; int Nz = 97;
  Nx+=3; Ny+=3; Nz+=3;
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
  fflush(stdout);
  if(me==0)
  {
      for (i=0; i<num_splines; i++)
      {
          int x, y, z;
          for (x=0; x<Nx; x++)
              for (y=0; y<Ny; y++)
                  for (z=0; z<Nz; z++)
                  {   j=x*(Ny*Nz)+y*Nz+z;
              data[j] = (x*100*100+y*100+z)*100+i;}
          lo[0]=lo[1]=lo[2]=0;
          hi[0]=Nx-1;hi[1]=Ny-1;hi[2]=Nz-1;
          lo[3]=hi[3]=i%num_splines;
          ld[0]=Ny;ld[1]=Nz;ld[2]=1;
          NGA_Put(g_a,lo,hi,data,ld);
      }
  }
  GA_Update_ghosts(g_a);
  GA_Sync();
  printf("done\n"),fflush(stdout);
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
  GA_Print_distribution(g_a);
  int low[16][4],high[16][4];
  for(i=0;i<nprocs;i++)
      NGA_Distribution(g_a,i,low[i],high[i]);
  srand ( time(NULL) );
  int k=GA_Nodeid();
  printf("%d: low[k]=%d high[k]=%d\n", GA_Nodeid(), low[k][2], high[k][2]);
  int unequal=0;
  for(i=0;i<1000;i++) {
      ix=rand_index(low[k][0],high[k][0]);
      if(ix+3>=dims[0]) ix=low[k][0];
      iy=rand_index(low[k][1],high[k][1]);
      if(iy+3>=dims[1]) iy=low[k][1];
      iz=rand_index(low[k][2],high[k][2]);
      if(iz+3>=dims[2]) iz=low[k][2];
      coefs_ga_get_3d(ga_coefs,coefs1,ix,iy,iz);
      long get_sum=mini_cube_sum(coefs1, ga_coefs->nsplines);
      long ghost_sum=coefs_ghost_access_3d(ga_coefs, coefs1, ix, iy, iz);
      if(get_sum!=ghost_sum) {
      printf("ixyz=\t%d\t%d\t%d\t", ix, iy, iz);
          printf("get_sum=%ld ghost_sum=%ld\n", get_sum, ghost_sum);
          unequal++;
      }
  }
  printf("unequal count=%d\n", unequal);
  free(coefs1);
  GA_Terminate();
  MPI_Finalize();
  return 0;
}

