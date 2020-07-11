#include <timeseries.h>

Timeseries::Timeseries(uint32_t maxLength)
{
  min = 1e12;
  max = -1e12;
  maxHistoryLength = maxLength;
}

bool Timeseries::push(float timestamp, float value)
{
  bool updateStatsNeeded = false;
  min = value < min ? value : min;
  max = value > max ? value : max;

  // try
  {
    if (data.size() >= maxHistoryLength)
    {
      updateStatsNeeded = true;
      data.erase(data.begin());
    }
    data.push_back(Point({timestamp, value}));

    if (updateStatsNeeded)
      updateStats();
  }
  // this could be out-of-memory situations or the block size is too big to be allocated
//   catch (const exception &e)
//   {
// #ifdef ARDUINO
//     Serial.printf("[ ERROR ] Timeseries::push(): %s\n", e.what());
// #else
//     printf("Error in Timeseries::push(): %s\n", e.what());
// #endif
//     return false;
//   }

  return true;
}

void Timeseries::updateStats()
{
  min = 1e12;
  max = -1e12;
  for (Point &p : data)
  {
    min = p.value < min ? p.value : min;
    max = p.value > max ? p.value : max;
  }
}

float Timeseries::mean()
{
  float mean = 0;

  if (data.size())
  {
    mean = data[0].value;
    for (PointIterator i = data.begin() + 1; i != data.end(); ++i)
    {
      mean += Point(*i).value;
    }
    mean = mean / data.size();
  }
  return mean;
}

uint32_t Timeseries::size()
{
  return data.size();
}

uint32_t Timeseries::capacity()
{
  return data.capacity();
}

// from: https://rosettacode.org/wiki/Ramer-Douglas-Peucker_line_simplification#C.2B.2B
float Timeseries::perpendicularDistance(const Point &pt, const Point &lineStart, const Point &lineEnd)
{
  float dx = lineEnd.time - lineStart.time;
  float dy = lineEnd.value - lineStart.value;

  //Normalise
  float mag = pow(pow(dx, 2.0) + pow(dy, 2.0), 0.5);
  if (mag > 0.0)
  {
    dx /= mag;
    dy /= mag;
  }

  float pvx = pt.time - lineStart.time;
  float pvy = pt.value - lineStart.value;

  //Get dot product (project pv onto normalized direction)
  float pvdot = dx * pvx + dy * pvy;

  //Scale line direction vector
  float dsx = pvdot * dx;
  float dsy = pvdot * dy;

  //Subtract this from pv
  float ax = pvx - dsx;
  float ay = pvy - dsy;

  return pow(pow(ax, 2.0) + pow(ay, 2.0), 0.5);
}

void Timeseries::ramerDouglasPeucker(const vector<Point> &pointList, float epsilon, vector<Point> &out)
{
  if (pointList.size() < 2)
    // throw invalid_argument("Not enough points to simplify");
    return;

  // Find the point with the maximum distance from line between start and end
  float dmax = 0.0;
  size_t index = 0;
  size_t end = pointList.size() - 1;
  for (size_t i = 1; i < end; i++)
  {
    float d = perpendicularDistance(pointList[i], pointList[0], pointList[end]);
    if (d > dmax)
    {
      index = i;
      dmax = d;
    }
  }

  // If max distance is greater than epsilon, recursively simplify
  if (dmax > epsilon)
  {
    // Recursive call
    vector<Point> recResults1;
    vector<Point> recResults2;
    vector<Point> firstLine(pointList.begin(), pointList.begin() + index + 1);
    vector<Point> lastLine(pointList.begin() + index, pointList.end());
    ramerDouglasPeucker(firstLine, epsilon, recResults1);
    ramerDouglasPeucker(lastLine, epsilon, recResults2);

    // Build the result list
    out.assign(recResults1.begin(), recResults1.end() - 1);
    out.insert(out.end(), recResults2.begin(), recResults2.end());
    if (out.size() < 2)
      // throw runtime_error("Problem assembling output");
      return;
  }
  else
  {
    //Just return start and end points
    out.clear();
    out.push_back(pointList[0]);
    out.push_back(pointList[end]);
  }
}

/**
* This will apply the Ramer-Douglas-Peucker algorithm to the dataset stored in the data-vector.
* @param epsilon Larger values will result in fewer data points
*/
int32_t Timeseries::compact(float epsilon)
{
  int32_t removedEntries = 0;
  vector<Point> pointListOut;

  if (data.size())
  {
    // try
    {
      ramerDouglasPeucker(data, epsilon, pointListOut);
      removedEntries = data.size() - pointListOut.size();
      data.assign(pointListOut.begin(), pointListOut.end());
      data.shrink_to_fit();
    }
//     catch (const std::exception &e)
//     {
// #ifdef ARDUINO
//       Serial.printf("[ ERROR ] Timeseries::compact(%f): %s\n", epsilon, e.what());
// #else
//       printf("Error in Timeseries::compact(%f): %s\n", epsilon, e.what());
// #endif
//       removedEntries = -1;
//     }
  }
  return removedEntries;
}

/**
 * Checks the timestamps and drops all entries that are older that given by maxAgeSeconds  
 */
int32_t Timeseries::trim(uint32_t currentTimeSeconds, uint32_t maxAgeSeconds)
{
  int32_t removedEntries = 0;
  // try
  {
    if (data.size())
    {
      PointIterator i = data.begin();
      while (Point(*i).time + maxAgeSeconds < currentTimeSeconds)
      {
        i++;
      }
      removedEntries = i - data.begin();
      // data.erase(data.begin(), i);
    }
  }
//   catch (const std::exception &e)
//   {
// #ifdef ARDUINO
//     Serial.printf("[ ERROR ] Timeseries::trim(%u, %u): %s\n", currentTimeSeconds, maxAgeSeconds, e.what());
//     removedEntries = -1;
// #else
//     printf("[ ERROR ] Timeseries::trim(%u, %u): %s\n", currentTimeSeconds, maxAgeSeconds, e.what());
// #endif
//   }

  return removedEntries;
}

/****************
 * A simple sliding window averaging function
 * @param samples the resulting window size is 2 * samples + 1
 */
void Timeseries::movingAverage(int32_t samples)
{
  vector<Point> tmp = data;
  float div = 1.;
  // try
  {
    for (int32_t i = 0; i < int32_t(data.size()); i++)
    {
      div = 1.;
      for (int32_t j = -samples; j <= samples; j++)
      {
        if (j != 0)
        {
          data.at(i).time += tmp.at(std::min(std::max(i + j, 0), int(data.size() - 1))).time;
          data.at(i).value += tmp.at(std::min(std::max(i + j, 0), int(data.size() - 1))).value;
          div += 1;
        }
      }
      data.at(i).time /= div;
      data.at(i).value /= div;
    }
  }
//   catch (const std::exception &e)
//   {
// #ifdef ARDUINO
//     Serial.printf("[ ERROR ] Timeseries::movingAverage(): %s\n", e.what());
// #else
//     printf("Error in Timeseries::movingAverage(): %s\n", e.what());
// #endif
//   }
}
