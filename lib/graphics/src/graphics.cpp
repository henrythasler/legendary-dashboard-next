#include <graphics.h>

Graphics::Graphics(void)
{
}

#ifdef ARDUINO
void Graphics::getTextBounds(Adafruit_GFX *display, Dimensions *dim, struct tm *timeinfo, const char *format)
{
    const char *f = format;
    if (!f)
    {
        f = "%c";
    }

    char buf[64];
    size_t written = strftime(buf, 64, f, timeinfo);

    int16_t x = 0, y = 0;

    if (written > 0)
    {
        display->getTextBounds(buf, 0, 0, &x, &y, &dim->width, &dim->height);
    }
}

void Graphics::getTextBounds(Adafruit_GFX *display, Dimensions *dim, const char *text)
{
    int16_t x = 0, y = 0;
    display->getTextBounds(text, 0, 0, &x, &y, &dim->width, &dim->height);
}

void Graphics::getTextBounds(Adafruit_GFX *display, Dimensions *dim, String text)
{
    int16_t x = 0, y = 0;
    display->getTextBounds(text, 0, 0, &x, &y, &dim->width, &dim->height);
}
#endif
