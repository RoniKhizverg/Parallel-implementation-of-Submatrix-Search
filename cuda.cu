#include <cuda_runtime.h>
#include <helper_cuda.h>
#include <string.h>
#include "cuda.h"


__device__ double calcMatchSum(int* picture, int* object, int i,int j,double matchingValue, int pictureSize, int objectSize) {
	double absSum = 0;
	int column = j;
	for (int k = 0; k < objectSize; k++, i++) {
		column = j;
		for (int x = 0; x < objectSize; x++, column++) {
		
			absSum = absSum + fabs((double)((picture[i * pictureSize + column] - object[k * objectSize + x]) / (double)picture[i * pictureSize + column]));
			
			/*** If we will use thr condition below, the result will be better, but the function will not consider as an "heavy function". ***/
			
			//if(absSum>matchingValue){
			//	return absSum;
			//}
		}
	}
	return absSum;
}

__global__ void calcAbsSum(int pictureSize,int objectSize,int* picture,int* object, double* absArray, int threadsPerBlock,double matchingValue) {
	double matcing = 0;

	for(int i = threadIdx.x; i < pictureSize; i += threadsPerBlock) // after 32 threads, we will return to the first thread again
	{
		for(int j = threadIdx.y; j < pictureSize; j += threadsPerBlock) // after 32 threads, we will return to the first thread again
		{
			matcing = calcMatchSum(picture, object, i, j ,matchingValue, pictureSize, objectSize);
			absArray[i*pictureSize + j] = matcing;
		}
	}
}

void findPairOnGPU( Element* picture,  Element** object , double matchingValue, Pair* pair, int* Count, int numOfObjects)
{
	pair->i =-1;
	pair->j = -1;
	pair->pictureID = picture->Id;
	pair->objectID = -1;
	pair-> isPair = 0;

	int foundObj = 0;
	
   	// Error code to check return values for CUDA calls
	cudaError_t err = cudaSuccess;
  
    	int *d_Picture;

   	// Allocate memory on GPU to copy the data from the host
    	err = cudaMalloc((void **)&d_Picture, (picture->dimention * picture->dimention) * sizeof(int));
    	if (err != cudaSuccess) {
			fprintf(stderr, "Failed to allocate device memory - %s\n", cudaGetErrorString(err));
			exit(EXIT_FAILURE);
		}
    	    	    	
    	// Copy data from host to the GPU memory
    	err = cudaMemcpy(d_Picture, picture->members, (picture->dimention * picture->dimention) * sizeof(int), cudaMemcpyHostToDevice);
    	if (err != cudaSuccess) {  
			fprintf(stderr, "Failed to copy data from host to device - %s\n", cudaGetErrorString(err));
			exit(EXIT_FAILURE);
		}
    	
    	for(int obj = 0; obj < numOfObjects; obj++) {
    		int *d_Object;
    		
	    	double *d_absArray;
	    	double *h_absArray;
	    	
	    	h_absArray = (double*)malloc((picture->dimention*picture->dimention)* sizeof(double));
	    	if (h_absArray == NULL) {
				printf("Problem to allocate memory\n");
				exit(0);
			}
	    	
			//Allocate memory on GPU to copy the data from the host
    		err = cudaMalloc((void **)&d_Object, object[obj]->dimention * object[obj]->dimention * sizeof(int));
    		if (err != cudaSuccess){
				fprintf(stderr, "Failed to copy data from host to device - %s\n", cudaGetErrorString(err));
				exit(EXIT_FAILURE);
			}
		
    		// Allocate memory on GPU to copy the data from the host
    		err = cudaMalloc((void **)&d_absArray, (picture->dimention*picture->dimention)* sizeof(double));
	    	if (err != cudaSuccess){
				fprintf(stderr, "Failed to copy data from host to device - %s\n", cudaGetErrorString(err));
				exit(EXIT_FAILURE);
			}

    		// Allocate memory on GPU to copy the data from the host
	    	err = cudaMemcpy(d_Object, object[obj]->members, (object[obj]->dimention * object[obj]->dimention) * sizeof(int), cudaMemcpyHostToDevice);
	    	if (err != cudaSuccess){
			fprintf(stderr, "Failed to copy data from host to device - %s\n", cudaGetErrorString(err));
			exit(EXIT_FAILURE);
			}

	    	// Launch the Kernel
	    	int threadsPerBlock = 32;
	    	if(picture->dimention <= threadsPerBlock)
	    	{
	    		dim3 dimBlock(picture->dimention, picture->dimention);
		    	calcAbsSum<<<1, dimBlock>>>(picture->dimention, object[obj]->dimention, d_Picture,d_Object, d_absArray,threadsPerBlock,matchingValue);
		    	err = cudaGetLastError();
				if (err != cudaSuccess){
					fprintf(stderr, "Failed to launch calcSum kernel (error code %s)!\n", cudaGetErrorString(err));
					exit(EXIT_FAILURE);
				}
			}
			else
			{
					dim3 dimBlock(threadsPerBlock, threadsPerBlock);
					calcAbsSum<<<1, dimBlock>>>(picture->dimention, object[obj]->dimention, d_Picture, d_Object, d_absArray,threadsPerBlock,matchingValue);
					err = cudaGetLastError();
				if (err != cudaSuccess){
					fprintf(stderr, "Failed to launch calcSum kernel (error code %s)!\n", cudaGetErrorString(err));
					exit(EXIT_FAILURE);
				}
			}
	    	
	    	
    		err = cudaMemcpy(h_absArray, d_absArray, (picture->dimention*picture->dimention) * sizeof(double), cudaMemcpyDeviceToHost);
			if (err != cudaSuccess){
				fprintf(stderr, "Failed to allocate device memory - %s\n", cudaGetErrorString(err));
				exit(EXIT_FAILURE);
			}
		 
			for(int i = 0; i < picture->dimention && !foundObj; i++ ){
		
				for(int j = 0; j <picture->dimention && !foundObj; j++){
			
					if(h_absArray[i*picture->dimention + j] <= matchingValue){
					 
						pair->i = i;
						pair->j = j;
						pair->pictureID = picture->Id;
						pair->objectID = object[obj]->Id;
						pair-> isPair = 1;
						foundObj=1;										
					}
				}
			}
	    	
	        err = cudaFree(d_Object);
			if (err != cudaSuccess){
				fprintf(stderr, "Failed to free device d_Object (error code %s)!\n", cudaGetErrorString(err));
				exit(EXIT_FAILURE);
			}
	        
	        err = cudaFree(d_absArray);
	        if (err != cudaSuccess){
				fprintf(stderr, "Failed to free device d_SumArray (error code %s)!\n", cudaGetErrorString(err));
				exit(EXIT_FAILURE);
			}
	        
	        free(h_absArray);
	        
	        if(foundObj)
	        	break;
	}
	*Count+=1;	 
	
    // Free allocated memory on GPU
    err = cudaFree(d_Picture);
    if (err != cudaSuccess){
		fprintf(stderr, "Failed to free device d_SumArray (error code %s)!\n", cudaGetErrorString(err));
		exit(EXIT_FAILURE);
	}
	
}

