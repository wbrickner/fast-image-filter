//
//  fif_macros.h
//  FastImageFilter
//
//  Created by Will Brickner on 3/10/19.
//  Copyright © 2019 Will Brickner. All rights reserved.
//

#ifndef fif_macros_h
#define fif_macros_h

#include <cmath>

using namespace std;

// these approaches avoids branches, so it's faster,
// especially on processors with MIN or MAX instructions
#define CLAMP_RGB(v) max(0, min(255, v))
#define GUARANTEE_ABOVE(above, value) max(above, value)
#define GUARANTEE_BELOW(below, value) min(below, value)

#endif
