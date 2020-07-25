#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>

#ifdef ARDUINO
#include <Adafruit_GFX.h>
#endif

using namespace std;

struct Dimensions {
    uint16_t width, height;
};

class Graphics
{
private:
public:
    Graphics(void);
#ifdef ARDUINO
    void getTextBounds(Adafruit_GFX *display, Dimensions *dim, struct tm * timeinfo, const char * format);
    void getTextBounds(Adafruit_GFX *display, Dimensions *dim, String text);
    void getTextBounds(Adafruit_GFX *display, Dimensions *dim, const char *text);
#endif
};
#endif