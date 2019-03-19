//
//  convenience_png++.cpp
//  FastImageFilter
//
//  Created by Will Brickner on 3/10/19.
//  Copyright © 2019 Will Brickner. All rights reserved.
//

#include "convenience_png++.hpp"

#include <thread>
#include "../utils/fif_macros.h"
#include <png++/image.hpp>
#include <png++/rgb_pixel.hpp>

void fif::image_from_linear(double *linear,
                           png::image<png::rgb_pixel> &img,
                           const uint16_t width,
                           const uint16_t height) {
    
    for (uint16_t y = 0; y < height; ++y) {
        for (uint16_t x = 0; x < width; ++x) {
            img.set_pixel(
                x, y,
                png::rgb_pixel(
                   // can't increment here, these additions happen in no guaranteed order
                   *(linear),
                   *(linear + 1),
                   *(linear + 2)
               )
            );
            
            linear += 3;
        }
    }
}

void linear_from_image_parallel(png::image<png::rgb_pixel> &img,
                                double *linear,
                                const uint16_t width,
                                const uint16_t height,
                                uint32_t start_index,
                                uint32_t length) {
    
    // jump to where we need to start working on this particular thread
    linear += start_index;
    // compute once where we need to stop
    const double *end_pointer = linear + length;
    
    uint16_t x = 0, y = 0;
    
    while (linear < end_pointer) {
        *(linear++) = img[y][x].red;
        *(linear++) = img[y][x].green;
        *(linear++) = img[y][x].blue;
        
        x++;
        if (x >= width) {
            x = 0;
            y++;
        }
    }
}

void join_thread(thread &t) { t.join(); }

void fif::linearize_image(png::image<png::rgb_pixel> &img,
                         double *linear,
                         const uint16_t width,
                         const uint16_t height,
                         uint8_t thread_count) {
    
    thread_count = (uint8_t)GUARANTEE_ABOVE(1, (int)thread_count);
    
    if (thread_count == 1) {
        for (uint16_t y = 0; y < height; ++y) {
            for (uint16_t x = 0; x < width; ++x) {
                *(linear++) = img[y][x].red;
                *(linear++) = img[y][x].green;
                *(linear++) = img[y][x].blue;
            }
        }
        return;
    }
    
    // we must dynamically thread it (oh jeez, oh boy, oh no)
    vector<thread>thread_vector;
    
    const uint32_t final_index = 3 * width * height;
    // be careful: multiply by 3 out front so we break on pixels.
    const uint32_t delta_index = 3 * ceil((double)(width * height) / (double)(thread_count));
    uint32_t start_index = 0;
    uint32_t length = delta_index;

    for (uint32_t t = 0; t < thread_count; t++) {
        if (start_index + length > final_index) {
            // this means we've reached the final thread, but let's
            // make sure it doesn't run out into other memory
            length = final_index - start_index;
        }
                
        // to pass by reference you need to wrap that variable in ref( ... )
        thread_vector.push_back(
            thread(linear_from_image_parallel, ref(img), linear, width, height, start_index, length)
        );
        
        start_index += delta_index;
    }
    
    // wait for all threads to finish
    for_each(thread_vector.begin(), thread_vector.end(), join_thread);
}

void fif::linearize_image(png::image<png::rgb_pixel> &img, double *linear) {
    const uint16_t width  = img.get_width();
    const uint16_t height = img.get_height();
    
    fif::linearize_image(img, linear, width, height);
}
