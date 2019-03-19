//
//  fast_image_filter.cpp
//  FastImageFilter
//
//  Created by Will Brickner on 3/8/19.
//  Copyright © 2019 Will Brickner. All rights reserved.
//

#include "fast_image_filter.hpp"
#include "utils/fif_macros.h"
#include <cmath>
#include <thread>
#include <fftw3.h>

using namespace std;

void multiply_all(double *double_arr, const uint32_t length, const double factor) {
    const double *end = double_arr + length + 1;

    while (double_arr != end) {
        *(double_arr++) *= factor;
    }
}

void transform_and_modify(const uint8_t channel,
                          const fftw_plan &plan,
                          const fftw_plan &inverse_plan,
                          double *frequency_space,
                          double *image_space,
                          const uint32_t total_pixels,
                          const double renormalization_factor,
                          void (*frequency_modifier)(const uint8_t channel,
                                                     double *frequency_space,
                                                     double *image_space)) {
                              
    // perform the FFT (image space => frequency space)
    fftw_execute(plan);

    // modify the output - fitler the top frequencies away, leaving only the specified portion of low frequencies
    frequency_modifier(channel, frequency_space, image_space);

    // perform the IFFT (frequency space => image space)
    fftw_execute(inverse_plan);

    // FFTW provides unnormalized transforms, so IFFT(FFT(X)) = (2w) * (2h) * X
    // We only want X, so renormalize the output: multiply each element by 1/(4 * w * h).
    multiply_all(image_space, total_pixels, renormalization_factor);
}

void combine_image_spaces(const fif::image &img,
                          const uint32_t length,
                          double *image_space_r,
                          double *image_space_g,
                          double *image_space_b) {
    
    double* destination_index = (double*)img.image_data;
    const double* end = destination_index + length + 1;

    while (destination_index < end) {
        *(destination_index++) = (double)CLAMP_RGB((int)*(image_space_r++));
        *(destination_index++) = (double)CLAMP_RGB((int)*(image_space_g++));
        *(destination_index++) = (double)CLAMP_RGB((int)*(image_space_b++));
    }
}


void split_into_channels(const fif::image &img,
                         const uint32_t length,
                         double *image_space_r,
                         double *image_space_g,
                         double *image_space_b) {

    double *source_pointer = (double *)img.image_data;
    const double *end = source_pointer + length + 1;
    
    while (source_pointer < end) {
        *(image_space_r++) = *(source_pointer++);
        *(image_space_g++) = *(source_pointer++);
        *(image_space_b++) = *(source_pointer++);
    }
}

void fif::filter_image(fif::image &img,
                  const bool self_optimize,
                  void (*frequency_modifier)(const uint8_t channel,
                                             double *frequency_space,
                                             double *image_space)) {
    
    // precompute image info
    const uint16_t width = img.width;
    const uint16_t height = img.height;
    const uint32_t total_pixels = width * height;

    // create an image space for each color channel
    double *image_space_r = (double *)fftw_malloc(sizeof(double) * total_pixels);
    double *image_space_g = (double *)fftw_malloc(sizeof(double) * total_pixels);
    double *image_space_b = (double *)fftw_malloc(sizeof(double) * total_pixels);

    // create a frequency space for each color channel
    double *frequency_space_r = (double *)fftw_malloc(sizeof(fftw_complex) * total_pixels);
    double *frequency_space_g = (double *)fftw_malloc(sizeof(fftw_complex) * total_pixels);
    double *frequency_space_b = (double *)fftw_malloc(sizeof(fftw_complex) * total_pixels);

    // define flags to control the under-the-hood behavior of FFTW
    const uint8_t transform_flags = FFTW_DESTROY_INPUT | (self_optimize ? FFTW_MEASURE : FFTW_ESTIMATE);

    // create the forward-tranform plans
    const fftw_plan plan_r = fftw_plan_r2r_2d((int)width, (int)height, image_space_r, frequency_space_r, FFTW_REDFT10, FFTW_REDFT10, transform_flags);
    const fftw_plan plan_g = fftw_plan_r2r_2d((int)width, (int)height, image_space_g, frequency_space_g, FFTW_REDFT10, FFTW_REDFT10, transform_flags);
    const fftw_plan plan_b = fftw_plan_r2r_2d((int)width, (int)height, image_space_b, frequency_space_b, FFTW_REDFT10, FFTW_REDFT10, transform_flags);

    // create the inverse-transform plans
    const fftw_plan plan_r_inverse = fftw_plan_r2r_2d((int)width, (int)height, frequency_space_r, image_space_r, FFTW_REDFT01, FFTW_REDFT01, transform_flags);
    const fftw_plan plan_g_inverse = fftw_plan_r2r_2d((int)width, (int)height, frequency_space_g, image_space_g, FFTW_REDFT01, FFTW_REDFT01, transform_flags);
    const fftw_plan plan_b_inverse = fftw_plan_r2r_2d((int)width, (int)height, frequency_space_b, image_space_b, FFTW_REDFT01, FFTW_REDFT01, transform_flags);

    // prepare inputs (important to do this after plan creation because FFTW will use the input as scratch space during setup if using self-optimization)
    split_into_channels(img, total_pixels, image_space_r, image_space_g, image_space_b);

    // declare this variable next to where we use it, so it's near in physical memory (maybe, hopefully)
    const double renormalization_factor = 0.25f / (double)(total_pixels);
                      
    // perform the fft in parallel on 3 seperate threads, each thread then modifies the output
    // it's structured this way so that if one thread lags behind, the others don't need to wait for it (until the very end)
    thread thread_r(transform_and_modify, 0, plan_r, plan_r_inverse, frequency_space_r, image_space_r, total_pixels, renormalization_factor, frequency_modifier);
    thread thread_g(transform_and_modify, 1, plan_g, plan_g_inverse, frequency_space_g, image_space_g, total_pixels, renormalization_factor, frequency_modifier);
    thread thread_b(transform_and_modify, 2, plan_b, plan_b_inverse, frequency_space_b, image_space_b, total_pixels, renormalization_factor, frequency_modifier);

    // wait until all threads are finished
    thread_r.join();
    thread_g.join();
    thread_b.join();

    // reassemble the image (combine the color channels)
    combine_image_spaces(img, total_pixels, image_space_r, image_space_g, image_space_b);

    // free the transform plans
    fftw_destroy_plan(plan_r);
    fftw_destroy_plan(plan_g);
    fftw_destroy_plan(plan_b);
    fftw_destroy_plan(plan_r_inverse);
    fftw_destroy_plan(plan_g_inverse);
    fftw_destroy_plan(plan_b_inverse);

    // and finally free the image and frequency spaces
    fftw_free(image_space_r);
    fftw_free(image_space_g);
    fftw_free(image_space_b);
    fftw_free(frequency_space_r);
    fftw_free(frequency_space_g);
    fftw_free(frequency_space_b);
}
