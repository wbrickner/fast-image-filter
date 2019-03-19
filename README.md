# FastImageFilter [Alpha]

FastImageFilter (FIF) applies spectral filters to images, emphasizing and deemphasizing structures at different length scales.  You write your filter (frequency modifier function), and FIF runs it (quickly). 

# Installation

Installation of FIF should be very straightforward. Simply copy the contents of the `./src/` folder somewhere accessible to your build system (probably inside your project).

## Dependencies

The following dependencies are **required**:

* **[FFTW](http://www.fftw.org/download.html)** - used for efficiently applying Fourier transforms under the hood.  You will need to link against `libfftw3.a`.

The following dependencies are **optional**:

These dependencies are recommended, because they can make your life easie. 
*FIF provides convenience methods to efficiently convert from PNG++ image representation to FIF image representation.*

* **[PNG++](https://www.nongnu.org/pngpp/)** - can used to load PNG images.  You will need to link against `libpng.a`, `libpng16.a`, or similar.

# Usage

Let's filter the red frequency space, eliminating the top 98% of frequencies, while emphasizing the bottom 2% (the lowest frequencies).

```C++
void frequency_modifier(const uint8_t channel,
							 const uint16_t width,
							 const uint16_t height, 
                        double *freq_space, 
                        double *image_space) {
	// only filter the red channel
	if (channel != 0) { return }
	
	const uint16_t threshold = width * 0.02f;
	
	// the freq_space is a linear array encoding a 2D frequency space
	// we want to filter out the large indices and magnify the low indices
	for (uint16_t y = 0, idx = 0; y < height; ++y) {
		for (uint16_t x = 0; x < width; ++x, ++idx) {
			if (x < threshold || y < threshold) {
				freq_space[idx] *= 2;
			} else {
				freq_space[idx] = 0;
			}
		}
	}
}
```

And now let's apply the filter:

```C++
int main() {
	// ...

	// create an image
	fif::image img = {
	    width,
	    height,
	    image_data_ptr
	};
	
	// apply the filter
	fif::filter_image(img, false, &frequency_modifier);
}
```

# Image Representation

FIF represents images as `fif::image` objects, which store the width, height, and a pointer to a linear array of interlaced image channels.

In the future, arbitrary channel structures will be supported. For now, only RGB is supported.

FIF provides several convenience functions to convert back and forth between intuitive data structures and the more performant internal representation.

**Example:** 

(You must install PNG++ to use these convenience functions)

```C++
#include "convenience_png++.hpp"
```

Provides the following functions in the `fif` namespace:

```C++
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
```

# License (MIT)

Copyright 2019 Will Brickner.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

