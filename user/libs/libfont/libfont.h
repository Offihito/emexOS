#pragma once

#include <stdint.h>

typedef enum {
    FONT8X12      = 0,
    FONT8X12_BOLD = 1,
} font_id_t;

// returns the glyph row bits for character c at row [0..11]
uint16_t font_glyph(font_id_t font, unsigned char c, int row);

// font dimensions
int font_w(font_id_t font); // always 8
int font_h(font_id_t font); // always 12