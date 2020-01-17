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

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <stdio.h>

#include "k-means.h"

static void initialize_centroids_float(size_t points, size_t dimension,
                                       uint8_t k, float data[points][dimension],
                                       float centroids[k][dimension]) {
  for (size_t i = 0; i < k; ++i) {
    long int randval = random();
    double random_position = (double)randval;
    random_position /= (double)RAND_MAX;
    random_position *= (double)points;
    size_t random_position_unsigned = (size_t)random_position;
    for (size_t j = 0; j < dimension; ++j) {
      centroids[i][j] = data[random_position_unsigned][j];
    }
  }
}

static void initialize_centroids_double(size_t points, size_t dimension,
                                        uint8_t k,
                                        double data[points][dimension],
                                        double centroids[k][dimension]) {
  for (size_t i = 0; i < k; ++i) {
    long int randval = random();
    double random_position = (double)randval;
    random_position /= (double)RAND_MAX;
    random_position *= (double)points;
    size_t random_position_unsigned = (size_t)random_position;
    for (size_t j = 0; j < dimension; ++j) {
      centroids[i][j] = data[random_position_unsigned][j];
    }
  }
}

size_t k_means_d(size_t points, size_t dimension, uint8_t k,
                 double data[restrict points][dimension],
                 uint8_t point_centroid_map[points]) {

  double(*centroids_temp)[dimension] = malloc(sizeof(double[k][dimension]));
  double(*centroids)[dimension] = malloc(sizeof(double[k][dimension]));
  size_t *centroids_point_count = malloc(k * sizeof(*centroids_point_count));

  initialize_centroids_double(points, dimension, k, data, centroids);

  bool has_converged;

  size_t convergence_iterations = 0;
  do {

    memset(centroids_point_count, 0, k * sizeof(*centroids_point_count));

    has_converged = true; // Assume convergence until proven otherwise
    // For every data
    for (size_t pos = 0; pos < points; ++pos) {

      uint8_t centroid_chosen = 0;
      double closest_centroid = HUGE_VAL;
      // Find the closest centroid
      for (uint8_t centro = 0; centro < k; ++centro) {
        double distance_square = 0.f;
        for (size_t dim = 0; dim < dimension; ++dim) {
          distance_square += (centroids[centro][dim] - data[pos][dim]) *
                             (centroids[centro][dim] - data[pos][dim]);
        }
        if (distance_square < closest_centroid) {
          closest_centroid = distance_square;
          centroid_chosen = centro;
        }
      }

      if (point_centroid_map[pos] != centroid_chosen)
        has_converged = false;
      point_centroid_map[pos] = centroid_chosen;
      centroids_point_count[centroid_chosen] += 1;

      if (centroids_point_count[centroid_chosen] == 1)
        for (size_t dim = 0; dim < dimension; ++dim)
          centroids_temp[centroid_chosen][dim] = data[pos][dim];
      else
        for (size_t dim = 0; dim < dimension; ++dim)
          centroids_temp[centroid_chosen][dim] += data[pos][dim];
    }

    for (uint8_t centro = 0; centro < k; ++centro) {
      if (centroids_point_count[centro] != 0) {
        double total_points = (double)centroids_point_count[centro];
        for (size_t dim = 0; dim < dimension; ++dim)
          centroids[centro][dim] = centroids_temp[centro][dim] / total_points;
      }
    }
    convergence_iterations += 1;

  } while (!has_converged);

  free(centroids_temp);
  free(centroids_point_count);
  free(centroids);

  return convergence_iterations;
}

extern uint32_t settle_at;
extern unsigned invalid_neighbours_up_to;

size_t k_means_f(size_t points, size_t dimension, uint8_t k,
                 float data[restrict points][dimension],
                 uint8_t point_centroid_map[points]) {

  float(*centroids_temp)[dimension] = malloc(sizeof(float[k][dimension]));
  size_t *centroids_point_count = malloc(k * sizeof(*centroids_point_count));
  float(*centroids)[dimension] = malloc(sizeof(float[k][dimension]));

  initialize_centroids_float(points, dimension, k, data, centroids);

  bool has_converged;

  size_t convergence_iterations = 0;

  uint32_t *niter_bound_centroid =
      calloc(points, sizeof(*niter_bound_centroid));

  do {

    memset(centroids_point_count, 0, k * sizeof(size_t));

    has_converged = true; // Assume convergence until proven otherwise
    // For every data
    for (size_t pos = 0; pos < points; ++pos) {

      if (niter_bound_centroid[pos] > settle_at)
        goto skip;

      uint8_t centroid_chosen = 0;
      float closest_centroid = HUGE_VALF;
      // Find the closest centroid
      for (uint8_t centro = 0; centro < k; ++centro) {
        float distance_square = 0.f;
        for (size_t dim = 0; dim < dimension; ++dim) {
          distance_square += (centroids[centro][dim] - data[pos][dim]) *
                             (centroids[centro][dim] - data[pos][dim]);
        }
        if (distance_square < closest_centroid) {
          closest_centroid = distance_square;
          centroid_chosen = centro;
        }
      }

      if (point_centroid_map[pos] != centroid_chosen) {
        has_converged = false;
        size_t lb = pos < invalid_neighbours_up_to ? 0 : pos - invalid_neighbours_up_to;
        size_t ub = pos + invalid_neighbours_up_to > points - 1 ? points - 1: pos + invalid_neighbours_up_to;
        for (size_t it = lb; it <= ub; ++it) {
          niter_bound_centroid[it] = 0;
        }
      }

      point_centroid_map[pos] = centroid_chosen;
    skip:
      niter_bound_centroid[pos]++;
      centroid_chosen = point_centroid_map[pos];
      centroids_point_count[centroid_chosen] += 1;

      if (centroids_point_count[centroid_chosen] == 1)
        for (size_t dim = 0; dim < dimension; ++dim)
          centroids_temp[centroid_chosen][dim] = data[pos][dim];
      else
        for (size_t dim = 0; dim < dimension; ++dim)
          centroids_temp[centroid_chosen][dim] += data[pos][dim];
    }

    for (uint8_t centro = 0; centro < k; ++centro) {
      if (centroids_point_count[centro] != 0) {
        float total_points = (float)centroids_point_count[centro];
        for (size_t dim = 0; dim < dimension; ++dim)
          centroids[centro][dim] = centroids_temp[centro][dim] / total_points;
      }
    }
    convergence_iterations += 1;

  } while (!has_converged);

  free(centroids_temp);
  free(centroids_point_count);
  free(centroids);
  free(niter_bound_centroid);

  return convergence_iterations;
}
