#ifndef _COEFS_GA_H_
#define _COEFS_GA_H_

#include <ga.h>
#include <assert.h>
#include "macdecls.h"
typedef struct {
  int Mx,My,Mz;//data dimensions
  int nsplines;
  int g_a;

  double sumt;
  int amount;
}ga_coefs_t;
void coefs_ga_get_3d(ga_coefs_t *ga_coefs,void *mini_cube,int x,int y,int z);
long mini_cube_sum(void* mini_cube, int nsplines);
void print_range(char *pre,int ndim,
                            int lo[], int hi[], char* post);
int rand_index(int low,int high);
double timing(int isBegin, const char msg[]);
long coefs_ghost_access_3d(int g_a, int x, int y, int z, int nsplines);
int coefs_ga_create_ghost(int Nx, int Ny, int Nz, int nsplines);
#endif
