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
#include "coefs_ga.h"

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
  //double value=9.0; GA_Fill(g_a,&value);
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
      long ghost_sum=coefs_ghost_access_3d(ga_coefs->g_a, ix, iy, iz, ga_coefs->nsplines);
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

