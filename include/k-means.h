/*
 * Copyright 2017 Maxime Schmitt <max.schmitt@math.unistra.fr>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __K_MEANS_H
#define __K_MEANS_H

#include <stdint.h>
#include <stdlib.h>

size_t k_means_f(size_t points, size_t dimension, uint8_t k,
                 float data[restrict points][dimension],
                 uint8_t point_to_centroid_map[points]);

size_t k_means_d(size_t points, size_t dimension, uint8_t k,
                 double data[restrict points][dimension],
                 uint8_t point_to_centroid_map[points]);

#define k_means(points, dims, k, data, ptcm)                                   \
  _Generic((data[0][0]), float                                                 \
           : k_means_f, double                                                 \
           : k_means_d)(points, dims, k, data, ptcm)

#endif // __K-MEANS_H
