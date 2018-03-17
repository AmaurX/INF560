#ifndef CUDA_FILTERS_H
#define CUDA_FILTERS_H

// extern "C" {
#include "structs.h"
// }

int cuda_test();

int cuda_gray_filter(struct pixel *pixelTab, int numPixels, int stream);

#endif