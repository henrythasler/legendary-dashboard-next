#ifndef GFX_H
#define GFX_H

#include <images.h>

struct Image{
	const uint8_t *color;
	const uint8_t *black;

  Image(const uint8_t *color, const uint8_t *black)
      : color(color), black(black)
  {
  }	
};

struct {
  Image background = Image(backgroundRed, backgroundBlack);
} images;

// struct {
// } icons;
#endif
