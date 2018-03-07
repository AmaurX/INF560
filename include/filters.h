/**
 * Filter module
 * Contains filtering functions to optimize
 */

#ifndef FILTER_H
#define FILTER_H

#include "main.h"

#define CONV(l, c, nb_c) \
    (l) * (nb_c) + (c)

void apply_gray_filter(animated_gif *image);
void apply_gray_line(animated_gif *image);
void apply_blur_filter(animated_gif *image, int size, int threshold);
void apply_sobel_filter(animated_gif *image);

void lined_gray_filter(struct pixel *framePixelTab, int height, int width, int lineMin, int lineMax);
void central_blur_filter(struct pixel *framePixelTab, int height, int width,  int size, int threshold);
void lined_sobelf(struct pixel *framePixelTab, int height, int width, int lineMin, int lineMax);

#endif
