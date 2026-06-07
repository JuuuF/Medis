#include "mapping.h"

#include "config.h"

// ----------------------------------------------

uint16_t XY(uint8_t x, uint8_t y) {
  uint8_t phys_y = (MATRIX_HEIGHT - 1) - y;

  if (x % 2 == 0) {
    return (uint16_t)x * MATRIX_HEIGHT + phys_y;
  }

  return (uint16_t)x * MATRIX_HEIGHT + ((MATRIX_HEIGHT - 1) - phys_y);
}
