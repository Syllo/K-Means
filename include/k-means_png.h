#ifndef K_MEANS_PNG_H_
#define K_MEANS_PNG_H_

#include <stdbool.h>
#include <stdint.h>

bool read_png(const char *filename, uint16_t **image, uint32_t *height,
              uint32_t *width);

bool write_grey_png(const char *filename, uint32_t height, uint32_t width,
                    uint8_t image[height][width]);

#endif // K_MEANS_PNG_H_
