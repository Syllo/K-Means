#include <errno.h>
#include <png.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PNG_BYTES_TO_CHECK 8
static bool check_if_png(FILE *fp) {
  unsigned char buf[PNG_BYTES_TO_CHECK];

  /* Read in some of the signature bytes. */
  if (fread(buf, 1, PNG_BYTES_TO_CHECK, fp) != PNG_BYTES_TO_CHECK)
    return false;

  /* Compare the first PNG_BYTES_TO_CHECK bytes of the signature.
   * Return nonzero (true) if they match.
   */
  return (!png_sig_cmp(buf, 0, PNG_BYTES_TO_CHECK));
}

bool read_png(const char *filename, uint16_t **image, uint32_t *height,
              uint32_t *width) {
  *image = NULL;
  FILE *png_file = fopen(filename, "rb");
  if (png_file == NULL) {
    int saved_errno = errno;
    fprintf(stderr, "Failed to open png file %s: ", filename);
    errno = saved_errno;
    perror(NULL);
    return false;
  }

  if (!check_if_png(png_file)) {
    fprintf(stderr, "The file does not start with the PNG magic bytes\n");
    fclose(png_file);
    return false;
  }

  png_structp png_ptr =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL) {
    fclose(png_file);
    return false;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    fclose(png_file);
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return false;
  }

  uint16_t **rows = NULL;
  if (setjmp(png_jmpbuf(png_ptr))) {
    /* Free all of the memory associated with the png_ptr and info_ptr. */
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(png_file);
    free(*image);
    free(rows);
    /* If we get here, we had a problem reading the file. */
    return false;
  }

  png_init_io(png_ptr, png_file);
  png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);

  png_read_info(png_ptr, info_ptr);

  int bit_depth, color_type, interlace_type;
  png_get_IHDR(png_ptr, info_ptr, width, height, &bit_depth, &color_type,
               &interlace_type, NULL, NULL);

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png_ptr);

  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) != 0)
    png_set_tRNS_to_alpha(png_ptr);

  if (bit_depth < 16)
    png_set_expand_16(png_ptr);

  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY)
    png_set_add_alpha(png_ptr, 0xffff, PNG_FILLER_AFTER);

  if (bit_depth < 16)
    png_set_expand_16(png_ptr);

  png_read_update_info(png_ptr, info_ptr);

  size_t row_bytes = png_get_rowbytes(png_ptr, info_ptr);
  if (row_bytes != *width * 8) {
    fprintf(stderr, "Wrong row bytes me %d, png %zu\n", *width * 8, row_bytes);
    exit(EXIT_FAILURE);
  }
  rows = malloc(*height * sizeof(*rows));
  *image = malloc(sizeof(uint16_t[*height][*width * 4]));
  for (png_uint_32 row_id = 0; row_id < *height; row_id++) {
    rows[row_id] = &(*image)[*width * 4 * row_id];
  }
  png_read_image(png_ptr, (unsigned char **)rows);
  png_read_end(png_ptr, info_ptr);
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  fclose(png_file);
  free(rows);
  return true;
}

// write 8bit grey values
bool write_grey_png(const char *filename, uint32_t height, uint32_t width,
                    uint8_t image[height][width]) {
  FILE *png_file = fopen(filename, "wb");
  if (png_file == NULL) {
    int saved_errno = errno;
    fprintf(stderr, "Failed to open file '%s' for writing: ", filename);
    errno = saved_errno;
    perror(NULL);
    return false;
  }
  png_structp png_ptr =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL) {
    fclose(png_file);
    return false;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    fclose(png_file);
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return false;
  }

  unsigned char **rows = malloc(height * sizeof(*rows));
  if (setjmp(png_jmpbuf(png_ptr))) {
    /* Free all of the memory associated with the png_ptr and info_ptr. */
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(png_file);
    free(rows);
    /* If we get here, we had a problem reading the file. */
    return false;
  }
  png_init_io(png_ptr, png_file);
  png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_GRAY,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png_ptr, info_ptr);
  for (uint32_t row_id = 0; row_id < height; row_id++) {
    rows[row_id] = &image[row_id][0];
  }
  png_write_image(png_ptr, rows);
  png_write_end(png_ptr, info_ptr);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(png_file);
  free(rows);
  return true;
}
