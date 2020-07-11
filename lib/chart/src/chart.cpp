#include <chart.h>

Chart::Chart(void)
{
}

#ifdef ARDUINO
void Chart::lineChart(GxEPD_Class *display,
                      Timeseries *timeseries,
                      uint16_t canvasLeft,
                      uint16_t canvasTop,
                      uint16_t canvasWidth,
                      uint16_t canvasHeight,
                      uint16_t lineColor,
                      bool drawDataPoints,
                      bool yAxisMinAuto,
                      bool yAxisMaxAuto,
                      float yAxisMin,
                      float yAxisMax)
{
    if (timeseries->data.size() > 1)
    {
        uint32_t tMin = timeseries->data.front().time;
        uint32_t tMax = timeseries->data.back().time;

        float dataMin = yAxisMinAuto ? timeseries->min : yAxisMin;
        float dataMax = yAxisMaxAuto ? timeseries->max : yAxisMax;
        float dataOffset = (dataMax + dataMin) / 2.;

        float pixelPerTime = float(canvasWidth) / max(float(tMax - tMin), float(1.));
        float pixelPerValue = float(canvasHeight) / (dataMax - dataMin + 0.01);

        uint16_t screenX1 = 0, screenY1 = 0, screenX2 = 0, screenY2 = 0;

        float t1 = 0, y1 = 0, t2 = 0, y2 = 0;
        for (vector<Point>::const_iterator i = timeseries->data.begin() + 1; i != timeseries->data.end(); ++i)
        {
            t1 = float(Point(*(i - 1)).time);
            y1 = Point(*(i - 1)).value - dataOffset;

            screenX1 = canvasLeft + int16_t((t1 - tMin) * pixelPerTime);
            screenY1 = canvasTop + canvasHeight / 2 - int16_t(y1 * pixelPerValue);

            t2 = float(Point(*i).time);
            y2 = Point(*i).value - dataOffset;

            screenX2 = canvasLeft + int16_t((t2 - tMin) * pixelPerTime);
            screenY2 = canvasTop + canvasHeight / 2 - int16_t(y2 * pixelPerValue);

            display->drawLine(screenX1, screenY1, screenX2, screenY2, lineColor);
            if (drawDataPoints)
                display->fillCircle(screenX1, screenY1, 2, lineColor);
        }
    }
}

/**
 * Draw bars to show signal strength of mobile network
 * 
 ******************************************************/
void Chart::signalBars(GxEPD_Class *display, int strength, int x, int y, int numBars, int barWidth, int barHeight, int heightDelta, int gapWidth, uint16_t strokeColor, uint16_t signalColor, uint16_t fillColor)
{
    int i;

    for (i = 0; i < numBars; i++)
    {
        if (strength > (int)((31 / (numBars + 1))) * (i + 1))
        {
            display->fillRect(x + i * (barWidth + gapWidth), y + (numBars - 1 - i) * heightDelta, barWidth, barHeight - (numBars - 1 - i) * heightDelta, signalColor);
        }
        else {
            display->fillRect(x + i * (barWidth + gapWidth), y + (numBars - 1 - i) * heightDelta, barWidth, barHeight - (numBars - 1 - i) * heightDelta, fillColor);            
        }
        display->drawRect(x + i * (barWidth + gapWidth), y + (numBars - 1 - i) * heightDelta, barWidth, barHeight - (numBars - 1 - i) * heightDelta, strokeColor);
    }
}

#endif
