//
//  convenience_png++.hpp
//  FastImageFilter
//
//  Created by Will Brickner on 3/10/19.
//  Copyright © 2019 Will Brickner. All rights reserved.
//

#ifndef convenience_png___hpp
#define convenience_png___hpp

#include <cstdint>              // to use uint32_t
#include <png++/image.hpp>
#include <png++/rgb_pixel.hpp>

namespace fif {

    // TODO: add dynamic parallelism to this function also
    void image_from_linear(double *linear,
                           png::image<png::rgb_pixel> &img,
                           const uint16_t width,
                           const uint16_t height);

    void linearize_image(png::image<png::rgb_pixel> &img,
                         double *linear,
                         const uint16_t width,
                         const uint16_t height,
                         uint8_t thread_count = 1);

    void linearize_image(png::image<png::rgb_pixel> &img, double *linear);

}
#endif
