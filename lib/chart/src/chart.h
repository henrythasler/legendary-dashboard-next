#ifndef CHART_H
#define CHART_H

#include <timeseries.h>

#ifdef ARDUINO
#include <GxEPD.h>
#include <GxGDEW042Z15/GxGDEW042Z15.h> // 4.2" b/w/r
#endif

using namespace std;

class Chart
{
private:
public:
    Chart(void);
#ifdef ARDUINO
    void lineChart(GxEPD_Class *display,
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
                   float yAxisMax = 100);

    void plotLineWidth(GxEPD_Class *display, int32_t x0, int32_t y0, int32_t x1, int32_t y1, float wd, uint16_t lineColor);
#endif
};
#endif