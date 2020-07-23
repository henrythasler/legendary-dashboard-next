#ifndef CHART_H
#define CHART_H

#include <timeseries.h>

#ifdef ARDUINO
#include <Adafruit_GFX.h>
#endif

using namespace std;

class Chart
{
private:
public:
    Chart(void);
#ifdef ARDUINO
    void lineChart(Adafruit_GFX *display,
                   Timeseries *timeseries,
                   uint16_t canvasLeft = 0,
                   uint16_t canvasTop = 0,
                   uint16_t canvasWidth = 300,
                   uint16_t canvasHeight = 400,
                   float linewidth = 1,
                   uint16_t lineColor = 0, // GxEPD_BLACK
                   bool drawDataPoints = false,
                   bool yAxisMinAuto = true,
                   bool yAxisMaxAuto = true,
                   float yAxisMin = 0,
                   float yAxisMax = 100,
                   bool xAxisMinAuto = true,
                   bool xAxisMaxAuto = true,
                   float xAxisMin = 0,
                   float xAxisMax = 1000);

    void plotLineWidth(Adafruit_GFX *display, int32_t x0, int32_t y0, int32_t x1, int32_t y1, float wd, uint16_t lineColor);
#endif
};
#endif