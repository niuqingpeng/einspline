#include <stdio.h>
#include <cuda.h>
#include <cuda_runtime.h>



extern "C" void cudaInit(size_t sizeA);
extern "C" void cudaFinalize();
extern "C" void putGPU(void* h_A, size_t sizeA);
extern "C" void getGPU(void* h_A, size_t sizeA);

void* d_A;


void cudaInit(size_t sizeA){

	//allocate memory on device
	cudaMalloc( (void**) &d_A, sizeA);
}

void putGPU(void* h_A, size_t sizeA){

	//copy host data from argument to device
	cudaMemcpy(d_A, h_A, sizeA, cudaMemcpyHostToDevice);
}


void getGPU(void* h_A, size_t sizeA){
	
	//copy data from device to argument array
	cudaMemcpy(h_A, d_A, sizeA, cudaMemcpyDeviceToHost);
}

void cudaFinalize(){

	//free device memory
	cudaFree(d_A);
}


