#ifndef STATISTICS_H
#define STATISTICS_H

#include <stdint.h>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <sys/time.h>
#include <algorithm>

#ifdef ARDUINO
#include <Arduino.h>
#include <FS.h>
#else
#include <stdio.h>
typedef FILE File; 
#endif

using namespace std;

struct Point
{
  float time;
  float value;

  Point(float time = 0, float value = 0)
      : time(time), value(value)
  {
  }

  Point operator+(const Point &a) const
  {
    return {a.time + time, a.value + value};
  }

  Point operator-(const Point &a) const
  {
    return {a.time - time, a.value - value};
  }

  Point operator*(const Point &a) const
  {
    return {a.time * time, a.value * value};
  }
};

typedef vector<Point>::iterator PointIterator;

#ifndef PI
#define PI 3.14159265358979323846
#endif

class Timeseries
{
public:
  float min;
  float max;
  uint32_t maxHistoryLength;

  vector<Point> data;

  Timeseries(uint32_t maxLength = 32);

  void updateStats(void);
  bool push(float timestamp, float value);
  uint32_t size();
  uint32_t capacity();
  float mean();
  int32_t compact(float epsilon = .2);
  int32_t trim(uint32_t currentTimeSeconds, uint32_t maxAgeSeconds = 604800);

  float perpendicularDistance(const Point &pt, const Point &lineStart, const Point &lineEnd);
  void ramerDouglasPeucker(const vector<Point> &pointList, float epsilon, vector<Point> &out);
  bool write(File* file);
  bool read(File* file);

  void movingAverage(int32_t samples = 5);
};
#endif