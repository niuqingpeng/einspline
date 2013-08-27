#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <mpi.h>
#include <unistd.h>
#include <ga.h>
#include "macdecls.h"
double drand48();
int main(int argc, char**argv)
{
  int nprocs, me;
  int i,j;
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD,&me);
  GA_Initialize();
  int Nx=111; int Ny=111; int Nz = 111;
  double data[Nx*Ny*Nz];
  int num_splines = 32;
  int g_a,dims[4]={Nx,Ny,Nz,num_splines},chunk[4]={-1,-1,-1,-1};
  int type=C_DBL;
  g_a=NGA_Create(type,4,dims,"Coefs",chunk);
  int lo[4],hi[4],ld[3];
  double value=9.0;
  GA_Fill(g_a,&value);
  GA_Print_distribution(g_a);
  fflush(stdout);
  sleep(9);
  if(me==0)
  {
      for (i=0; i<num_splines*10; i++) 
      {
          for (j=0; j<Nx*Ny*Nz; j++) 
              data[j] = (drand48()-0.5);
          lo[0]=lo[1]=lo[2]=0;
          hi[0]=Nx-1;hi[1]=Ny-1;hi[2]=Nz-1;
          lo[3]=hi[3]=i%num_splines;
          ld[0]=Ny;ld[1]=Nz;ld[2]=1;
          NGA_Put(g_a,lo,hi,data,ld);
      }
  }
  printf("done"),fflush(stdout);
  MPI_Barrier(MPI_COMM_WORLD);
  sleep(9);
  GA_Terminate();
  MPI_Finalize();
  return 0;
}

