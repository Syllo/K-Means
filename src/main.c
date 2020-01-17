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

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "k-means.h"
#include "k-means_png.h"
#include "time_measurement.h"

static void rand_init_data_f(size_t num_values, size_t dimension,
                             float array[num_values][dimension],
                             float max_of_generated_value) {
  for (size_t i = 0; i < num_values; ++i) {
    for (size_t j = 0; j < dimension; ++j) {
      array[i][j] = random() / (float)RAND_MAX * max_of_generated_value;
    }
  }
}

static void rand_init_data_d(size_t num_values, size_t dimension,
                             double array[num_values][dimension],
                             double max_of_generated_value) {
  for (size_t i = 0; i < num_values; ++i) {
    for (size_t j = 0; j < dimension; ++j) {
      array[i][j] = random() / (double)RAND_MAX * max_of_generated_value;
    }
  }
}

static struct option opt_options[] = {
    {"input-png", required_argument, 0, 'i'},
    {"output-png", required_argument, 0, 'o'},
    {"num-centroids", required_argument, 0, 'c'},
    {"random-data", required_argument, 0, 'r'},
    {"random-seed", required_argument, 0, 's'},
    {"help", no_argument, 0, 'h'},
    {"compare", required_argument, 0, 'C'},
    {"settle_skip", required_argument, 0, 'S'},
    {"settle_invalid_neighbour", required_argument, 0, 'I'},
    {0, 0, 0, 0}};

static const char options[] = ":i:o:c:r:d:m:s:hS:I:C:";

static const char help_string[] =
    "Options:"
    "\n  -i --input-png        : The png file to partition"
    "\n  -o --output-png       : The result of the partitioning (greyscale)"
    "\n  -c --num-centroids    : The number of partitions"
    "\n  -r --random-data      : Partition randomly generated data"
    "\n  -d --random-data-dims : The dimensions of the randomly generated data"
    "\n  -m --random-max       : Maximum value of random data (default 250.)"
    "\n  -s --random-seed      : The random seed used by the pseudo-random "
    "generator to"
    "\n                       initalize the algorithm and the random data"
    "\n  -h --help             : Print this help";

uint32_t settle_at = UINT32_MAX;
unsigned invalid_neighbours_up_to = 0;

int main(int argc, char **argv) {
  unsigned random_seed = 42;
  char *png_input_file = NULL;
  char *png_output_file = NULL;
  char *file_to_compare = NULL;
  bool use_double = false;
  size_t num_dims = 1;
  uint8_t num_centroids = 4;
  size_t num_points = 0;
  double max_rand_val = 250.;

  while (true) {
    int sscanf_return;
    int optchar = getopt_long(argc, argv, options, opt_options, NULL);
    if (optchar == -1)
      break;
    switch (optchar) {
    case 'i':
      png_input_file = optarg;
      break;
    case 'o':
      png_output_file = optarg;
      break;
    case 'c':
      sscanf_return = sscanf(optarg, "%" SCNu8, &num_centroids);
      if (sscanf_return == EOF || sscanf_return == 0 || num_centroids == 0) {
        fprintf(stderr,
                "Please enter a positive integer for the number of centroids "
                "instead of \"-%c %s\"\n",
                optchar, optarg);
      }
      break;
    case 'r':
      sscanf_return = sscanf(optarg, "%zu", &num_points);
      if (sscanf_return == EOF || sscanf_return == 0) {
        fprintf(stderr,
                "Please enter a positive integer for the number of random data"
                "instead of \"-%c %s\"\n",
                optchar, optarg);
      }
      break;
    case 'd':
      sscanf_return = sscanf(optarg, "%zu", &num_dims);
      if (sscanf_return == EOF || sscanf_return == 0 || num_dims == 0) {
        fprintf(stderr,
                "Please enter a positive integer for the number of dimensions "
                "of the random data instead of \"-%c %s\"\n",
                optchar, optarg);
      }
      break;
    case 'm':
      sscanf_return = sscanf(optarg, "%lf", &max_rand_val);
      if (sscanf_return == EOF || sscanf_return == 0) {
        fprintf(stderr,
                "Please enter a valid floating point number for the maximum "
                "random value instead of \"-%c %s\"\n",
                optchar, optarg);
      }
      break;
    case 's':
      sscanf_return = sscanf(optarg, "%u", &random_seed);
      if (sscanf_return == EOF || sscanf_return == 0) {
        fprintf(stderr,
                "Please enter a positive integer for the random seed"
                "instead of \"-%c %s\"\n",
                optchar, optarg);
      }
      break;
    case 'C':
      file_to_compare = optarg;
      break;
    case 'I':
      sscanf_return = sscanf(optarg, "%u", &invalid_neighbours_up_to);
      if (sscanf_return == EOF || sscanf_return == 0) {
        fprintf(stderr,
                "Please enter a positive integer number for the "
                "number of neighbours to invalidate "
                "instead of \"-%c %s\"\n",
                optchar, optarg);
        invalid_neighbours_up_to = 0;
      }
      break;
    case 'S':
      sscanf_return = sscanf(optarg, "%" SCNu32, &settle_at);
      if (sscanf_return == EOF || sscanf_return == 0) {
        fprintf(stderr,
                "Please enter a positive integer number for the "
                "settle skip value "
                "instead of \"-%c %s\"\n",
                optchar, optarg);
        settle_at = UINT32_MAX;
      }
      break;
    case 'h':
      printf("Usage: %s <options>\n%s\n", argv[0], help_string);
      return EXIT_SUCCESS;
    case ':':
      if (optopt == 'o') {
        png_output_file = "KMEAN_Image.png";
      } else {
        fprintf(stderr, "Option %c requires an argument\n", optopt);
        exit(EXIT_FAILURE);
      }
      break;
    default:
      fprintf(stderr, "Unrecognized option %c\n", optopt);
      exit(EXIT_FAILURE);
      break;
    }
  }
  srandom(random_seed);

  if (png_input_file != NULL) // PNG has 4 dims (RGBA)
    num_dims = 4;

  float(*data_f)[num_dims] = NULL;
  double(*data_d)[num_dims] = NULL;

  if (file_to_compare) {
    num_dims = 4;

    if (png_input_file == NULL) {
      fprintf(stderr, "Two files are needed for comparison (use -i and -C)\n");
      return EXIT_FAILURE;
    }
    uint32_t height = 0, width = 0, height2 = 0, width2 = 0;
    uint16_t *image = NULL;
    uint16_t *image2 = NULL;
    bool read_success = read_png(png_input_file, &image, &height, &width);
    bool read_success2 = read_png(file_to_compare, &image2, &height2, &width2);
    if (!read_success || !read_success2) {
      fprintf(stderr, "libpng was unable to read the image files\n");
      exit(EXIT_FAILURE);
    }
    if (width != width2) {
      fprintf(stderr, "The images to compare must have the same width\n");
      exit(EXIT_FAILURE);
    }
    if (height != height2) {
      fprintf(stderr, "The images to compare must have the same height\n");
      exit(EXIT_FAILURE);
    }
    num_points = width;
    num_points *= height;
    uint16_t(*image_tab)[width][4] = (uint16_t(*)[width][4])image;
    uint16_t(*image_tab2)[width][4] = (uint16_t(*)[width][4])image2;
    size_t differences = 0;
    for (size_t i = 0; i < height; ++i) {
      for (size_t j = 0; j < width; ++j) {
        bool equal = true;
        for (unsigned k = 0; k < 4; ++k) {
          if (image_tab[i][j][k] != image_tab2[i][j][k])
            equal = false;
        }
        if (!equal) {
          differences++;
        }
      }
    }
    fprintf(stdout, "Num differences: %zu\n", differences);
    fprintf(stdout, "Num Points: %u\n", width * height);
    fprintf(stdout, "Error: %e%%\n",
            differences / (double)(width * height) * 100.);
    free(image);
    free(image2);
    return EXIT_SUCCESS;
  }

  uint32_t height = 0, width = 0;
  if (png_input_file != NULL) { // Read data from png file
    uint16_t *image = NULL;
    bool read_success = read_png(png_input_file, &image, &height, &width);
    if (!read_success)
      exit(EXIT_FAILURE);
    num_points = width;
    num_points *= height;
    if (use_double) {
      data_d = malloc(sizeof(double[num_points][num_dims]));
      double(*data_d_tab)[width][4] = (double(*)[width][4])data_d;
      uint16_t(*image_tab)[width][4] = (uint16_t(*)[width][4])image;
      for (size_t i = 0; i < width; ++i) {
        for (size_t j = 0; j < height; ++j) {
          data_d_tab[i][j][0] = image_tab[i][j][0];
          data_d_tab[i][j][1] = image_tab[i][j][1];
          data_d_tab[i][j][2] = image_tab[i][j][2];
          data_d_tab[i][j][3] = image_tab[i][j][3];
        }
      }
    } else {
      data_f = malloc(sizeof(float[num_points][num_dims]));
      float(*data_f_tab)[width][4] = (float(*)[width][4])data_f;
      uint16_t(*image_tab)[width][4] = (uint16_t(*)[width][4])image;
      for (size_t i = 0; i < height; ++i) {
        for (size_t j = 0; j < width; ++j) {
          data_f_tab[i][j][0] = image_tab[i][j][0];
          data_f_tab[i][j][1] = image_tab[i][j][1];
          data_f_tab[i][j][2] = image_tab[i][j][2];
          data_f_tab[i][j][3] = image_tab[i][j][3];
        }
      }
    }
    free(image);
  } else { // Init random
    if (num_points == 0) {
      fprintf(stdout, "Neither PNG file nor random data size have been "
                      "selected.\nExiting as nothing needs to be done.\n");
      return EXIT_SUCCESS;
    }
    if (use_double) {
      data_d = malloc(sizeof(double([num_points][num_dims])));
      rand_init_data_d(num_points, num_dims, data_d, max_rand_val);
    } else {
      data_f = malloc(sizeof(float([num_points][num_dims])));
      rand_init_data_f(num_points, num_dims, data_f, (float)max_rand_val);
    }
  }

  uint8_t *point_centroid_map = calloc(num_points, sizeof(*point_centroid_map));
  size_t steps_to_convergence = 0;

  time_measure startTime, endTime;
  get_current_time(&startTime);
  if (use_double)
    steps_to_convergence = k_means(num_points, num_dims, num_centroids, data_d,
                                   point_centroid_map);
  else
    steps_to_convergence = k_means(num_points, num_dims, num_centroids, data_f,
                                   point_centroid_map);
  get_current_time(&endTime);

  if (png_input_file != NULL && png_output_file != NULL) {
    uint8_t(*out_image)[width] = malloc(sizeof(uint8_t[height][width]));
    uint8_t multiplier = UINT8_MAX / num_centroids;
    uint8_t(*pcm_tab)[width] = (uint8_t(*)[width])point_centroid_map;
    for (size_t i = 0; i < height; ++i) {
      for (size_t j = 0; j < width; ++j) {
        out_image[i][j] = pcm_tab[i][j] * multiplier;
      }
    }
    write_grey_png(png_output_file, height, width, out_image);
    free(out_image);
  }

  fprintf(stdout, "Converged in %zu steps\nKernel time %.4fs\n",
          steps_to_convergence, measuring_difftime(startTime, endTime));

  free(point_centroid_map);
  if (use_double)
    free(data_d);
  else
    free(data_f);

  return EXIT_SUCCESS;
}
