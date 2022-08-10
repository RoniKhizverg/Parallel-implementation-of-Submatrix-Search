
#ifndef CUDA__H__
#define CUDA__H__

#include "functions.h"

void findPairOnGPU( Element* picture, Element** object , double matchingValue, Pair* pair, int* Count, int numOfObjects);

#endif /* CUDA_H_ */
