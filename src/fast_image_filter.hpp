//
//  fast_image_filter.hpp
//  FastImageFilter
//
//  Created by Will Brickner on 3/8/19.
//  Copyright © 2019 Will Brickner. All rights reserved.
//

#ifndef FastImageFilter_hpp
#define FastImageFilter_hpp

#include <cstdint> // to use uint8_t, uint16_t, and uint32_t

// NOTE: self-optimization happens before image manipulation and
//       can signficantly shorten execution time on very large
//       input.  However, this is an expensive task, and so the
//       threshold after which this becomes bennificial depends on,
//       and is likely unique to your hardware.

namespace fif {
    struct image {
        const uint16_t width;
        const uint16_t height;
        const double *image_data;
    };
    
    void filter_image(image &img,
                      const bool self_optimize,
                      void (*frequency_modifier)(const uint8_t channel,
                                                 double *frequency_space,
                                                 double *image_space));
}

#endif
