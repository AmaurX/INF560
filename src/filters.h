/**
 * Filter module
 * Contains filtering functions to optimize
 */

#ifndef FILTER_H
#define FILTER_H

#define CONV(l, c, nb_c) \
    (l) * (nb_c) + (c)

void apply_gray_filter(animated_gif *image);
void apply_gray_line(animated_gif *image);
void apply_blur_filter(animated_gif *image, int size, int threshold);
void apply_sobel_filter(animated_gif *image);

#endif