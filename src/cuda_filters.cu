#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// CUDA runtime
#include <cuda_runtime.h>

// includes
#include <helper_functions.h> // helper for shared functions common to CUDA Samples
#include <helper_cuda.h>	  // helper functions for CUDA error checking and initialization

#include <cuda.h>

extern "C"
{
	#include "cuda_filters.h"
	#include "main.h"
}

__global__ void kern_gray_filter(struct pixel* pixelTab, char* checkTab, int numPixels)
{
	int i = blockIdx.x * blockDim.x + threadIdx.x;
	int nProc = blockDim.x * gridDim.x;
	//1D grid-striding loop
	while(i < numPixels)
	{
		checkTab[i]++;
		int moy = (pixelTab[i].r + pixelTab[i].g +  pixelTab[i].b) / 3;
		if (moy < 0)
			moy = 0;
		if (moy > 255)
			moy = 255;
		pixelTab[i].r = moy;
		pixelTab[i].g = moy;
		pixelTab[i].b = moy;
		i+=nProc;
	}
}

extern "C" int cuda_test()
{
	printf("cuda_test was succesfully linked\n");
	return 0;
}

extern "C" int cuda_gray_filter(struct pixel* pixelTab, int numPixels, int stream)
{
	struct pixel* d_pixelTab;
	size_t tabSize = numPixels * sizeof(struct pixel);
	// for debug purpose : check that each elt is processed once
	size_t checkTabSize = numPixels * sizeof(char);
	char* checkTab = (char*) calloc(numPixels, sizeof(char));
	char* d_checkTab;

	int nThreads = (int) min(1024, (int)sqrt(numPixels));
	int nBlocks = (int) min(65535, (int)(1+numPixels/nThreads));
	dim3 gridDim;
	gridDim.x = nBlocks;
	dim3 blockDim;
	blockDim.x = nThreads;

	cudaEvent_t start, end;
	cudaEventCreate(&start);
	cudaEventCreate(&end);
	checkCudaErrors(cudaMalloc((void **)&d_pixelTab, tabSize));
	checkCudaErrors(cudaMalloc((void **)&d_checkTab, checkTabSize));

	//initialize the device memory
	checkCudaErrors(cudaMemcpy(d_pixelTab, pixelTab, tabSize,	cudaMemcpyHostToDevice));
	checkCudaErrors(cudaMemcpy(d_checkTab, checkTab, checkTabSize,	cudaMemcpyHostToDevice));
	
	cudaEventRecord(start, 0);
	kern_gray_filter<<<gridDim, blockDim>>>(d_pixelTab, d_checkTab, numPixels);
	cudaEventRecord(end, 0);
	
	//copy back
	checkCudaErrors(cudaMemcpy(pixelTab, d_pixelTab, tabSize,	cudaMemcpyDeviceToHost));
	checkCudaErrors(cudaMemcpy(checkTab, d_checkTab, checkTabSize,	cudaMemcpyDeviceToHost));
	
	// check each elt is treated only once
	int totalCheck = 0;
	for(int i = 0 ; i < numPixels ; i++)
	{
		int diff = abs(checkTab[i] -1);
		if(diff != 0)
		{
			dbprintf("pixl %d check failed : %d\n", i, checkTab[i]);
		}
		totalCheck += diff;
	}

	cudaEventSynchronize(end);
	float duration;
	cudaEventElapsedTime(&duration, start, end);

	dbprintf("gray check = %d in %.2f ms (%d)\n", totalCheck, duration, numPixels );

	return 0;

}
