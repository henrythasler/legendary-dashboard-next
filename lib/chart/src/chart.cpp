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
                      float lineWidth,
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

            plotLineWidth(display, screenX1, screenY1, screenX2, screenY2, lineWidth, lineColor);
            if (drawDataPoints)
                display->fillCircle(screenX1, screenY1, 2, lineColor);
        }
    }
}

// from: http://members.chello.at/~easyfilter/bresenham.html
void Chart::plotLineWidth(GxEPD_Class *display, int32_t x0, int32_t y0, int32_t x1, int32_t y1, float wd, uint16_t lineColor)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx - dy, e2, x2, y2; /* error value e_xy */
    float ed = dx + dy == 0 ? 1 : sqrt((float)dx * dx + (float)dy * dy);

    for (wd = (wd + 1) / 2;;)
    { /* pixel loop */
        if ((abs(err - dx + dy) / ed - wd + 1) <= 0.5)
            display->writePixel(x0, y0, lineColor);
        e2 = err;
        x2 = x0;
        if (2 * e2 >= -dx)
        { /* x step */
            for (e2 += dy, y2 = y0; e2 < ed * wd && (y1 != y2 || dx > dy); e2 += dx)
            {
                y2 += sy;
                if ((abs(e2) / ed - wd + 1) <= 0.5)
                    display->writePixel(x0, y2, lineColor);
            }
            if (x0 == x1)
                break;
            e2 = err;
            err -= dy;
            x0 += sx;
        }
        if (2 * e2 <= dy)
        { /* y step */
            for (e2 = dx - e2; e2 < ed * wd && (x1 != x2 || dx < dy); e2 += dy)
            {
                x2 += sx;
                if ((abs(e2) / ed - wd + 1) <= 0.5)
                    display->writePixel(x2, y0, lineColor);
            }
            if (y0 == y1)
                break;
            err += dx;
            y0 += sy;
        }
    }
}

#endif
