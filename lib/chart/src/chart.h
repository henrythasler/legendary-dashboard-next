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
                   uint16_t lineColor = 0, // GxEPD_BLACK
                   bool drawDataPoints = false,
                   bool yAxisMinAuto = true,
                   bool yAxisMaxAuto = true,
                   float yAxisMin = 0,
                   float yAxisMax = 100);

    void signalBars(GxEPD_Class *display,
                    int strength,
                    int x,
                    int y,
                    int numbars,
                    int barwidth,
                    int barheight,
                    int heightdelta,
                    int gap,
                    uint16_t strokeColor = 0,     // BLACK
                    uint16_t signalColor = 0, // WHITE
                    uint16_t fillColor = 0xFFFF); // WHITE
#endif
};
#endif