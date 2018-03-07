/**
 * Filter module
 * Contains filtering functions to optimize
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "main.h"

#include "filters.h"

void apply_gray_filter(animated_gif *image)
{
    int i, j;
    pixel **p;

    p = image->p;
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic)
#endif
    for (i = 0; i < image->n_images; i++)
    {
        for (j = 0; j < image->width[i] * image->height[i]; j++)
        {
            int moy;

            // moy = p[i][j].r/4 + ( p[i][j].g * 3/4 ) ;
            moy = (p[i][j].r + p[i][j].g + p[i][j].b) / 3;
            if (moy < 0)
                moy = 0;
            if (moy > 255)
                moy = 255;

            p[i][j].r = moy;
            p[i][j].g = moy;
            p[i][j].b = moy;
        }
    }
}

void lined_gray_filter(struct pixel *framePixelTab, int height, int width, int lineMin, int lineMax)
{
    pixel *p;

    p = framePixelTab;
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic) default(None)
#endif
    for (int j = lineMin * width; j < lineMax * width; j++)
    {
        int moy;
        // moy = p[i][j].r/4 + ( p[i][j].g * 3/4 ) ;
        moy = (p[j].r + p[j].g + p[j].b) / 3;
        if (moy < 0)
            moy = 0;
        if (moy > 255)
            moy = 255;

        p[j].r = moy;
        p[j].g = moy;
        p[j].b = moy;
    }
}

// void apply_gray_line(animated_gif *image)
// {
//     int i, j, k;
//     pixel **p;

//     p = image->p;

//     for (i = 0; i < image->n_images; i++)
//     {
//         for (j = 0; j < 10; j++)
//         {
//             for (k = image->width[i] / 2; k < image->width[i]; k++)
//             {
//                 p[i][CONV(j, k, image->width[i])].r = 0;
//                 p[i][CONV(j, k, image->width[i])].g = 0;
//                 p[i][CONV(j, k, image->width[i])].b = 0;
//             }
//         }
//     }
// }

void central_blur_filter(struct pixel *framePixelTab, int height, int width,  int size, int threshold)
{
    int j, k;
    int end = 0;
    int n_iter = 0;

    pixel *p;
    pixel *new;

    /* Get the pixels of all images */
    p = framePixelTab;

    /* Process all images */
    n_iter = 0;

    /* Allocate array of new pixels */
    new = (pixel *)malloc(width * height * sizeof(pixel));

    /* Perform at least one blur iteration */
    do
    {
        end = 1;
        n_iter++;

        /* Apply blur on top part of image (10%) */
        for (j = size; j < height / 10 - size; j++)
        {
            for (k = size; k < width - size; k++)
            {
                int stencil_j, stencil_k;
                int t_r = 0;
                int t_g = 0;
                int t_b = 0;

                for (stencil_j = -size; stencil_j <= size; stencil_j++)
                {
                    for (stencil_k = -size; stencil_k <= size; stencil_k++)
                    {
                        t_r += p[CONV(j + stencil_j, k + stencil_k, width)].r;
                        t_g += p[CONV(j + stencil_j, k + stencil_k, width)].g;
                        t_b += p[CONV(j + stencil_j, k + stencil_k, width)].b;
                    }
                }

                new[CONV(j, k, width)].r = t_r / ((2 * size + 1) * (2 * size + 1));
                new[CONV(j, k, width)].g = t_g / ((2 * size + 1) * (2 * size + 1));
                new[CONV(j, k, width)].b = t_b / ((2 * size + 1) * (2 * size + 1));
            }
        }

        /* Copy the middle part of the image */
        for (j = height / 10 - size; j < height * 0.9 + size; j++)
        {
            for (k = size; k < width - size; k++)
            {
                new[CONV(j, k, width)].r = p[CONV(j, k, width)].r;
                new[CONV(j, k, width)].g = p[CONV(j, k, width)].g;
                new[CONV(j, k, width)].b = p[CONV(j, k, width)].b;
            }
        }

        /* Apply blur on the bottom part of the image (10%) */
        for (j = height * 0.9 + size; j < height - size; j++)
        {
            for (k = size; k < width - size; k++)
            {
                int stencil_j, stencil_k;
                int t_r = 0;
                int t_g = 0;
                int t_b = 0;

                for (stencil_j = -size; stencil_j <= size; stencil_j++)
                {
                    for (stencil_k = -size; stencil_k <= size; stencil_k++)
                    {
                        t_r += p[CONV(j + stencil_j, k + stencil_k, width)].r;
                        t_g += p[CONV(j + stencil_j, k + stencil_k, width)].g;
                        t_b += p[CONV(j + stencil_j, k + stencil_k, width)].b;
                    }
                }

                new[CONV(j, k, width)].r = t_r / ((2 * size + 1) * (2 * size + 1));
                new[CONV(j, k, width)].g = t_g / ((2 * size + 1) * (2 * size + 1));
                new[CONV(j, k, width)].b = t_b / ((2 * size + 1) * (2 * size + 1));
            }
        }

        for (j = 1; j < height - 1; j++)
        {
            for (k = 1; k < width - 1; k++)
            {

                float diff_r;
                float diff_g;
                float diff_b;

                diff_r = (new[CONV(j, k, width)].r - p[CONV(j, k, width)].r);
                diff_g = (new[CONV(j, k, width)].g - p[CONV(j, k, width)].g);
                diff_b = (new[CONV(j, k, width)].b - p[CONV(j, k, width)].b);

                if (diff_r > threshold || -diff_r > threshold ||
                    diff_g > threshold || -diff_g > threshold ||
                    diff_b > threshold || -diff_b > threshold)
                {
                    end = 0;
                }

                p[CONV(j, k, width)].r = new[CONV(j, k, width)].r;
                p[CONV(j, k, width)].g = new[CONV(j, k, width)].g;
                p[CONV(j, k, width)].b = new[CONV(j, k, width)].b;
            }
        }

    } while (threshold > 0 && !end);

    // printf( "Nb iter for image %d\n", n_iter ) ;

    free(new);
}

void apply_blur_filter(animated_gif *image, int size, int threshold)
{
    int i, j, k;
    int width, height;
    int end = 0;
    int n_iter = 0;

    pixel **p;
    pixel *new;

    /* Get the pixels of all images */
    p = image->p;

    /* Process all images */
    for (i = 0; i < image->n_images; i++)
    {
        n_iter = 0;
        width = image->width[i];
        height = image->height[i];

        /* Allocate array of new pixels */
        new = (pixel *)malloc(width * height * sizeof(pixel));

        /* Perform at least one blur iteration */
        do
        {
            end = 1;
            n_iter++;

            /* Apply blur on top part of image (10%) */
            for (j = size; j < height / 10 - size; j++)
            {
                for (k = size; k < width - size; k++)
                {
                    int stencil_j, stencil_k;
                    int t_r = 0;
                    int t_g = 0;
                    int t_b = 0;

                    for (stencil_j = -size; stencil_j <= size; stencil_j++)
                    {
                        for (stencil_k = -size; stencil_k <= size; stencil_k++)
                        {
                            t_r += p[i][CONV(j + stencil_j, k + stencil_k, width)].r;
                            t_g += p[i][CONV(j + stencil_j, k + stencil_k, width)].g;
                            t_b += p[i][CONV(j + stencil_j, k + stencil_k, width)].b;
                        }
                    }

                    new[CONV(j, k, width)].r = t_r / ((2 * size + 1) * (2 * size + 1));
                    new[CONV(j, k, width)].g = t_g / ((2 * size + 1) * (2 * size + 1));
                    new[CONV(j, k, width)].b = t_b / ((2 * size + 1) * (2 * size + 1));
                }
            }

            /* Copy the middle part of the image */
            for (j = height / 10 - size; j < height * 0.9 + size; j++)
            {
                for (k = size; k < width - size; k++)
                {
                    new[CONV(j, k, width)].r = p[i][CONV(j, k, width)].r;
                    new[CONV(j, k, width)].g = p[i][CONV(j, k, width)].g;
                    new[CONV(j, k, width)].b = p[i][CONV(j, k, width)].b;
                }
            }

            /* Apply blur on the bottom part of the image (10%) */
            for (j = height * 0.9 + size; j < height - size; j++)
            {
                for (k = size; k < width - size; k++)
                {
                    int stencil_j, stencil_k;
                    int t_r = 0;
                    int t_g = 0;
                    int t_b = 0;

                    for (stencil_j = -size; stencil_j <= size; stencil_j++)
                    {
                        for (stencil_k = -size; stencil_k <= size; stencil_k++)
                        {
                            t_r += p[i][CONV(j + stencil_j, k + stencil_k, width)].r;
                            t_g += p[i][CONV(j + stencil_j, k + stencil_k, width)].g;
                            t_b += p[i][CONV(j + stencil_j, k + stencil_k, width)].b;
                        }
                    }

                    new[CONV(j, k, width)].r = t_r / ((2 * size + 1) * (2 * size + 1));
                    new[CONV(j, k, width)].g = t_g / ((2 * size + 1) * (2 * size + 1));
                    new[CONV(j, k, width)].b = t_b / ((2 * size + 1) * (2 * size + 1));
                }
            }

            for (j = 1; j < height - 1; j++)
            {
                for (k = 1; k < width - 1; k++)
                {

                    float diff_r;
                    float diff_g;
                    float diff_b;

                    diff_r = (new[CONV(j, k, width)].r - p[i][CONV(j, k, width)].r);
                    diff_g = (new[CONV(j, k, width)].g - p[i][CONV(j, k, width)].g);
                    diff_b = (new[CONV(j, k, width)].b - p[i][CONV(j, k, width)].b);

                    if (diff_r > threshold || -diff_r > threshold ||
                        diff_g > threshold || -diff_g > threshold ||
                        diff_b > threshold || -diff_b > threshold)
                    {
                        end = 0;
                    }

                    p[i][CONV(j, k, width)].r = new[CONV(j, k, width)].r;
                    p[i][CONV(j, k, width)].g = new[CONV(j, k, width)].g;
                    p[i][CONV(j, k, width)].b = new[CONV(j, k, width)].b;
                }
            }

        } while (threshold > 0 && !end);

        // printf( "Nb iter for image %d\n", n_iter ) ;

        free(new);
    }
}

void apply_sobel_filter(animated_gif *image)
{
    int i, j, k;
    int width, height;

    pixel **p;

    p = image->p;

    for (i = 0; i < image->n_images; i++)
    {
        width = image->width[i];
        height = image->height[i];

        pixel *sobel;

        sobel = (pixel *)malloc(width * height * sizeof(pixel));
        // #pragma omp parallel for schedule(dynamic) (ne marche pas...)
        for (j = 1; j < height - 1; j++)
        {
            for (k = 1; k < width - 1; k++)
            {
                int pixel_blue_no, pixel_blue_n, pixel_blue_ne;
                int pixel_blue_so, pixel_blue_s, pixel_blue_se;
                int pixel_blue_o, pixel_blue, pixel_blue_e;

                float deltaX_blue;
                float deltaY_blue;
                float val_blue;

                pixel_blue_no = p[i][CONV(j - 1, k - 1, width)].b;
                pixel_blue_n = p[i][CONV(j - 1, k, width)].b;
                pixel_blue_ne = p[i][CONV(j - 1, k + 1, width)].b;
                pixel_blue_so = p[i][CONV(j + 1, k - 1, width)].b;
                pixel_blue_s = p[i][CONV(j + 1, k, width)].b;
                pixel_blue_se = p[i][CONV(j + 1, k + 1, width)].b;
                pixel_blue_o = p[i][CONV(j, k - 1, width)].b;
                pixel_blue = p[i][CONV(j, k, width)].b;
                pixel_blue_e = p[i][CONV(j, k + 1, width)].b;

                deltaX_blue = -pixel_blue_no + pixel_blue_ne - 2 * pixel_blue_o + 2 * pixel_blue_e - pixel_blue_so + pixel_blue_se;

                deltaY_blue = pixel_blue_se + 2 * pixel_blue_s + pixel_blue_so - pixel_blue_ne - 2 * pixel_blue_n - pixel_blue_no;

                val_blue = sqrt(deltaX_blue * deltaX_blue + deltaY_blue * deltaY_blue) / 4;

                if (val_blue > 50)
                {
                    sobel[CONV(j, k, width)].r = 255;
                    sobel[CONV(j, k, width)].g = 255;
                    sobel[CONV(j, k, width)].b = 255;
                }
                else
                {
                    sobel[CONV(j, k, width)].r = 0;
                    sobel[CONV(j, k, width)].g = 0;
                    sobel[CONV(j, k, width)].b = 0;
                }
            }
        }

        // #pragma omp parallel for schedule(dynamic) (doit marcher)
        for (j = 1; j < height - 1; j++)
        {
            for (k = 1; k < width - 1; k++)
            {
                p[i][CONV(j, k, width)].r = sobel[CONV(j, k, width)].r;
                p[i][CONV(j, k, width)].g = sobel[CONV(j, k, width)].g;
                p[i][CONV(j, k, width)].b = sobel[CONV(j, k, width)].b;
            }
        }

        free(sobel);
    }
}

/**
 * lined version of the sobelf filter
 * the intermediate buffer is smaller (only the treated part size is allocated)
 **/
void lined_sobelf(struct pixel *framePixelTab, int height, int width, int lineMin, int lineMax)
{
    int j, k;

    pixel *p;

    p = framePixelTab;

    pixel *sobel;

    sobel = (pixel *)malloc(width * (lineMax - lineMin) * sizeof(pixel));

    // 1-pixel border is not treated
    lineMin = (lineMin < 1 ? 1 : lineMin);                   //max(1, lineMin)
    lineMax = (lineMax > height - 1 ? height - 1 : lineMax); //min(height-1, lineMax)
    // #pragma omp parallel for schedule(dynamic) (ne marche pas...)
    for (j = lineMin; j < lineMax; j++)
    {
        for (k = 1; k < width - 1; k++)
        {
            int pixel_blue_no, pixel_blue_n, pixel_blue_ne;
            int pixel_blue_so, pixel_blue_s, pixel_blue_se;
            int pixel_blue_o, /*pixel_blue,*/ pixel_blue_e;

            float deltaX_blue;
            float deltaY_blue;
            float val_blue;

            pixel_blue_n = p[CONV(j - 1, k, width)].b;
            pixel_blue_ne = p[CONV(j - 1, k + 1, width)].b;
            pixel_blue_no = p[CONV(j - 1, k - 1, width)].b;
            pixel_blue_so = p[CONV(j + 1, k - 1, width)].b;
            pixel_blue_s = p[CONV(j + 1, k, width)].b;
            pixel_blue_se = p[CONV(j + 1, k + 1, width)].b;
            pixel_blue_o = p[CONV(j, k - 1, width)].b;
            // pixel_blue = p[CONV(j, k, width)].b;
            pixel_blue_e = p[CONV(j, k + 1, width)].b;

            deltaX_blue = -pixel_blue_no + pixel_blue_ne - 2 * pixel_blue_o + 2 * pixel_blue_e - pixel_blue_so + pixel_blue_se;

            deltaY_blue = pixel_blue_se + 2 * pixel_blue_s + pixel_blue_so - pixel_blue_ne - 2 * pixel_blue_n - pixel_blue_no;

            val_blue = sqrt(deltaX_blue * deltaX_blue + deltaY_blue * deltaY_blue) / 4;

            int coord = CONV(j - lineMin, k, width);
            if (val_blue > 50)
            {
                sobel[coord].r = 255;
                sobel[coord].g = 255;
                sobel[coord].b = 255;
            }
            else
            {
                sobel[coord].r = 0;
                sobel[coord].g = 0;
                sobel[coord].b = 0;
            }
        }
    }

    // #pragma omp parallel for schedule(dynamic) (doit marcher)
    for (j = lineMin; j < lineMax; j++)
    {
        for (k = 1; k < width - 1; k++)
        {
            int localCoord = CONV(j - lineMin, k, width);
            int totalCoord = CONV(j, k, width);
            p[totalCoord].r = sobel[localCoord].r;
            p[totalCoord].g = sobel[localCoord].g;
            p[totalCoord].b = sobel[localCoord].b;
        }
    }

    free(sobel);
}