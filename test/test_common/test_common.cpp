#include <unity.h>
#include <timeseries.h>

#ifndef ARDUINO
#include <stdio.h>
#include <chrono>
#endif

typedef std::vector<float> float_vec_t;

void test_function_timeseries_initial(void)
{
  Timeseries series;
  TEST_ASSERT_EQUAL_FLOAT(float(1e12), series.min);
  TEST_ASSERT_EQUAL_FLOAT(float(-1e12), series.max);
  TEST_ASSERT_EQUAL_FLOAT(0, series.mean());
  TEST_ASSERT_EQUAL_UINT32(0, series.size());
}

void test_function_timeseries_single(void)
{
  Timeseries series(8);
  series.push(0, 2);
  TEST_ASSERT_EQUAL_FLOAT(2., series.min);
  TEST_ASSERT_EQUAL_FLOAT(2., series.max);
  TEST_ASSERT_EQUAL_FLOAT(2., series.mean());
  TEST_ASSERT_EQUAL_UINT32(1, series.size());
}

void test_function_timeseries_simple(void)
{
  Timeseries series(8);
  series.push(0, 0);
  series.push(1, 1);
  TEST_ASSERT_EQUAL_FLOAT(0., series.min);
  TEST_ASSERT_EQUAL_FLOAT(1., series.max);
  TEST_ASSERT_EQUAL_FLOAT(.5, series.mean());
  TEST_ASSERT_EQUAL_UINT32(2, series.size());
}

void test_function_timeseries_limiter(void)
{
  Timeseries series(4);
  series.push(0, 0);
  series.push(1, 1);
  series.push(2, 2);
  series.push(3, 3);
  series.push(4, 4);
  series.push(5, 5);

  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(2., series.min, "series.min");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(5., series.max, "series.max");
  TEST_ASSERT_EQUAL_FLOAT_MESSAGE(3.5, series.mean(), "series.mean()");
  TEST_ASSERT_EQUAL_UINT32(4, series.size());

  TEST_ASSERT_EQUAL_FLOAT(2., series.data.at(0).time);
  TEST_ASSERT_EQUAL_FLOAT(2., series.data.at(0).value);
  TEST_ASSERT_EQUAL_FLOAT(5., series.data.at(3).time);
  TEST_ASSERT_EQUAL_FLOAT(5., series.data.at(3).value);
}

void test_function_timeseries_huge(void)
{
  int maxEntries = 20000;
#ifdef ARDUINO
  maxEntries = 5000;
#endif

  Timeseries series(maxEntries);
  for (int i = 0; i < 50000; i++)
  {
    series.push(i, i % 100);
  }
  TEST_ASSERT_EQUAL_FLOAT(0., series.min);
  TEST_ASSERT_EQUAL_FLOAT(99., series.max);
  TEST_ASSERT_EQUAL_FLOAT(49.5, series.mean());
  TEST_ASSERT_EQUAL_UINT32(maxEntries, series.size());

#ifndef ARDUINO
  char message[64];
  int size = sizeof(series.data) + sizeof(Point) * series.data.capacity();
  snprintf(message, 64, "size=%u", size);
  TEST_MESSAGE(message);
#endif
}

void test_function_timeseries_rdp_synth1(void)
{
  Timeseries series;

  vector<Point> examplePoint;
  vector<Point> pointListOut;

  examplePoint.push_back(Point({1, 1.}));
  examplePoint.push_back(Point({2, 2.}));
  examplePoint.push_back(Point({3, 3.}));
  examplePoint.push_back(Point({4, 4.}));

  series.ramerDouglasPeucker(examplePoint, 1.0, pointListOut);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(2, pointListOut.size(), "pointListOut.size()");

  TEST_ASSERT_EQUAL_FLOAT(1., pointListOut.at(0).time);
  TEST_ASSERT_EQUAL_FLOAT(1., pointListOut.at(0).value);

  TEST_ASSERT_EQUAL_FLOAT(4., pointListOut.at(1).time);
  TEST_ASSERT_EQUAL_FLOAT(4., pointListOut.at(1).value);
}

void test_function_timeseries_rdp_synth2(void)
{
  Timeseries series;

  vector<Point> examplePoint;
  vector<Point> pointListOut;
  examplePoint.push_back(Point({5, 0.}));
  examplePoint.push_back(Point({4, 0.}));
  examplePoint.push_back(Point({3, 0.}));
  examplePoint.push_back(Point({3, 1.}));
  examplePoint.push_back(Point({3, 2.}));

  series.ramerDouglasPeucker(examplePoint, 1.0, pointListOut);

  TEST_ASSERT_EQUAL_INT32_MESSAGE(3, pointListOut.size(), "pointListOut.size()");

  TEST_ASSERT_EQUAL_FLOAT(5., pointListOut.at(0).time);
  TEST_ASSERT_EQUAL_FLOAT(0., pointListOut.at(0).value);

  TEST_ASSERT_EQUAL_FLOAT(3., pointListOut.at(1).time);
  TEST_ASSERT_EQUAL_FLOAT(0., pointListOut.at(1).value);

  TEST_ASSERT_EQUAL_FLOAT(3., pointListOut.at(2).time);
  TEST_ASSERT_EQUAL_FLOAT(2., pointListOut.at(2).value);
}

// from https://github.com/LukaszWiktor/series-reducer
void test_function_timeseries_rdp_math(void)
{
  Timeseries series;
  vector<Point>::const_iterator i;

  float_vec_t exampleFlat;

  vector<Point> examplePoint;
  vector<Point> pointListOut;

  for (uint32_t x = 0; x <= 80; x++)
  {
    examplePoint.push_back(Point({float(x), float(cos((float(x) / 20.) * (float(x) / 20.) - 1.)) * float(20.)}));
    exampleFlat.push_back(cos((float(x) / 20.) * (float(x) / 20.) - 1) * 20);
  }

  series.ramerDouglasPeucker(examplePoint, .2, pointListOut);
  pointListOut.shrink_to_fit();

#ifndef ARDUINO
  char message[64];
  int sizePoint = sizeof(examplePoint) + sizeof(Point) * examplePoint.capacity();
  int sizePointPack = sizeof(pointListOut) + sizeof(Point) * pointListOut.capacity();
  int sizeFlat = sizeof(exampleFlat) + sizeof(float) * exampleFlat.capacity();

  snprintf(message, 64, "sizePoint=%u vs. sizeFlat=%u vs. pointAfterRDP=%u", sizePoint, sizeFlat, sizePointPack);
  TEST_MESSAGE(message);
#endif

  TEST_ASSERT_EQUAL_INT32_MESSAGE(39, pointListOut.size(), "pointListOut.size()");
}

void test_function_timeseries_compact_math(void)
{
  Timeseries series(256);
  float_vec_t exampleFlat;

  for (int x = 0; x <= 80; x++)
  {
    series.push(x, cos((float(x) / 20.) * (float(x) / 20.) - 1) * 20);
  }

#ifndef ARDUINO
  int before = sizeof(series.data) + sizeof(Point) * series.data.capacity();
#endif

  int32_t removed = series.compact(0.2);
  TEST_ASSERT_EQUAL_INT32(42, removed);

#ifndef ARDUINO
  int after = sizeof(series.data) + sizeof(Point) * series.data.capacity();
  char message[64];
  snprintf(message, 64, "before=%u vs. after=%u", before, after);
  TEST_MESSAGE(message);
#endif

  TEST_ASSERT_EQUAL_INT32_MESSAGE(39, series.data.size(), "series.data.size()");
}

void test_function_timeseries_compact_throw(void)
{
  Timeseries series;
  series.push(0, 0);
  int32_t removed = series.compact();
  TEST_ASSERT_EQUAL_INT32(-1, removed);
  TEST_ASSERT_EQUAL_INT32_MESSAGE(1, series.data.size(), "series.data.size()");
}

void test_function_timeseries_compact_huge(void)
{
  try
  {
    Timeseries series(5000);
    for (uint32_t x = 1; x <= 100000; x++)
    {
      series.push(x - 1, 1);
      if (!(x % 100))
        series.compact();
    }
    TEST_ASSERT_EQUAL_INT32_MESSAGE(2, series.data.size(), "series.data.size()");

    TEST_ASSERT_EQUAL_FLOAT(0., series.data.front().time);
    TEST_ASSERT_EQUAL_FLOAT(1., series.data.front().value);
    TEST_ASSERT_EQUAL_FLOAT(99999., series.data.back().time);
    TEST_ASSERT_EQUAL_FLOAT(1., series.data.back().value);
  }
  catch (const std::exception &e)
  {
#ifdef ARDUINO
    Serial.printf("Error in Statistics::update(): %s\n", e.what());
#else
    printf("Error in Statistics::update(): %s\n", e.what());
#endif
  }
}

void test_function_timeseries_trim_simple(void)
{
  Timeseries series(10);
  series.push(0, 0);
  series.push(1, 1);
  series.push(2, 2);
  series.push(3, 3);
  series.push(4, 4);
  series.push(5, 5);

  TEST_ASSERT_EQUAL_UINT32(6, series.size());
  uint32_t removed = series.trim(6, 4);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(2, removed, "removed");
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(4, series.size(), "size()");
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(8, series.capacity(), "capacity()");
  TEST_ASSERT_EQUAL_FLOAT(2., series.data.at(0).time);
  TEST_ASSERT_EQUAL_FLOAT(2., series.data.at(0).value);
  TEST_ASSERT_EQUAL_FLOAT(5., series.data.at(3).time);
  TEST_ASSERT_EQUAL_FLOAT(5., series.data.at(3).value);
}

void test_function_timeseries_trim_none(void)
{
  Timeseries series(10);
  series.push(0, 0);
  series.push(1, 1);
  series.push(2, 2);
  series.push(3, 3);
  series.push(4, 4);
  series.push(5, 5);

  TEST_ASSERT_EQUAL_UINT32(6, series.size());
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(8, series.capacity(), "capacity() before");
  uint32_t removed = series.trim(6, 10);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(0, removed, "removed");
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(6, series.size(), "size()");
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(8, series.capacity(), "capacity() after trim()");
  TEST_ASSERT_EQUAL_FLOAT(0., series.data.at(0).time);
  TEST_ASSERT_EQUAL_FLOAT(0., series.data.at(0).value);
  TEST_ASSERT_EQUAL_FLOAT(5., series.data.at(5).time);
  TEST_ASSERT_EQUAL_FLOAT(5., series.data.at(5).value);
}

void test_function_timeseries_trim_compact_simple(void)
{
  Timeseries series(10);
  series.push(0, 0);
  series.push(1, 1);
  series.push(2, 2);
  series.push(3, 3);
  series.push(4, 4);
  series.push(5, 5);

  TEST_ASSERT_EQUAL_UINT32(6, series.size());
  uint32_t removed = series.trim(6, 4);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(2, removed, "removed");
  bool compactResult = series.compact();
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(2, series.size(), "size()");
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(2, series.capacity(), "capacity()");
  TEST_ASSERT_EQUAL_FLOAT(2., series.data.at(0).time);
  TEST_ASSERT_EQUAL_FLOAT(2., series.data.at(0).value);
  TEST_ASSERT_EQUAL_FLOAT(5., series.data.at(1).time);
  TEST_ASSERT_EQUAL_FLOAT(5., series.data.at(1).value);
}

void test_function_timeseries_average_identity(void)
{
  Timeseries series(10);
  series.push(0, 0);
  series.push(1, 1);
  series.push(2, 2);
  series.push(3, 3);
  series.push(4, 4);
  series.push(5, 5);
  series.movingAverage(1);
  
  TEST_ASSERT_EQUAL_UINT32(6, series.size());
//   for (Point &p : series.data)
//   {
// #ifdef ARDUINO
//     Serial.printf("%f/%f\n", p.time, p.value);
// #else
//     printf("%f/%f\n", p.time, p.value);
// #endif    
//   }    
  TEST_ASSERT_EQUAL_FLOAT(1./3, series.data.at(0).time);
  TEST_ASSERT_EQUAL_FLOAT(1./3, series.data.at(0).value);
  TEST_ASSERT_EQUAL_FLOAT(1., series.data.at(1).time);
  TEST_ASSERT_EQUAL_FLOAT(1., series.data.at(1).value);
  TEST_ASSERT_EQUAL_FLOAT(2., series.data.at(2).time);
  TEST_ASSERT_EQUAL_FLOAT(2., series.data.at(2).value);    
  TEST_ASSERT_EQUAL_FLOAT(3., series.data.at(3).time);
  TEST_ASSERT_EQUAL_FLOAT(3., series.data.at(3).value);
  TEST_ASSERT_EQUAL_FLOAT(4., series.data.at(4).time);
  TEST_ASSERT_EQUAL_FLOAT(4., series.data.at(4).value);
  TEST_ASSERT_EQUAL_FLOAT(4.+2./3, series.data.at(5).time);
  TEST_ASSERT_EQUAL_FLOAT(4.+2./3, series.data.at(5).value);    
}

void test_function_timeseries_average_step(void)
{
  Timeseries series(20);
  for(int i=0;i<20;i++){
    if(i<10)
      series.push(i, 0);  
      else
            series.push(i, 1);  

  }
  series.movingAverage(2);
  TEST_ASSERT_EQUAL_UINT32(20, series.size());

//   for (Point &p : series.data)
//   {
// #ifdef ARDUINO
//     Serial.printf("%f/%f\n", p.time, p.value);
// #else
//     printf("%f/%f\n", p.time, p.value);
// #endif    
//   }  

  TEST_ASSERT_EQUAL_FLOAT(0.6, series.data.at(0).time);
  TEST_ASSERT_EQUAL_FLOAT(0., series.data.at(0).value);
  TEST_ASSERT_EQUAL_FLOAT(9., series.data.at(9).time);
  TEST_ASSERT_EQUAL_FLOAT(0.4, series.data.at(9).value);
  TEST_ASSERT_EQUAL_FLOAT(10., series.data.at(10).time);
  TEST_ASSERT_EQUAL_FLOAT(0.6, series.data.at(10).value);
  TEST_ASSERT_EQUAL_FLOAT(18.4, series.data.at(19).time);
  TEST_ASSERT_EQUAL_FLOAT(1., series.data.at(19).value);    
}

void test_function_timeseries_average_huge(void)
{
  Timeseries series(5000);
  for(int i=0;i<5000;i++){
      series.push(i, i%10);  
  }
  series.movingAverage(10);
  TEST_ASSERT_EQUAL_UINT32(5000, series.size());

//   for(int i=0;i<50;i++){
// #ifdef ARDUINO
//     Serial.printf("%f/%f\n", series.data.at(i).time, series.data.at(i).value);
// #else
//     printf("%f/%f\n", series.data.at(i).time, series.data.at(i).value);
// #endif    
//   }    
}

void process(void)
{
  UNITY_BEGIN();
  RUN_TEST(test_function_timeseries_initial);
  RUN_TEST(test_function_timeseries_single);
  RUN_TEST(test_function_timeseries_simple);
  RUN_TEST(test_function_timeseries_limiter);
  RUN_TEST(test_function_timeseries_huge);
  RUN_TEST(test_function_timeseries_rdp_synth1);
  RUN_TEST(test_function_timeseries_rdp_synth2);
  RUN_TEST(test_function_timeseries_rdp_math);
  RUN_TEST(test_function_timeseries_compact_math);
  RUN_TEST(test_function_timeseries_compact_throw);
  RUN_TEST(test_function_timeseries_compact_huge);
  RUN_TEST(test_function_timeseries_trim_simple);
  RUN_TEST(test_function_timeseries_trim_none);
  RUN_TEST(test_function_timeseries_trim_compact_simple);
  RUN_TEST(test_function_timeseries_average_identity);
  RUN_TEST(test_function_timeseries_average_step);
  RUN_TEST(test_function_timeseries_average_huge);
  UNITY_END();
}

#ifdef ARDUINO
#include <Arduino.h>
#define LED_BUILTIN (13) // LED is connected to IO13

void setup()
{
  // NOTE!!! Wait for >2 secs
  // if board doesn't support software reset via Serial.DTR/RTS
  delay(2000);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  Serial.printf("Heap: %u KiB free\n", ESP.getFreeHeap() / 1024);
  process();
}

void loop()
{
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
}

#else

int main(int argc, char **argv)
{
  auto start = std::chrono::high_resolution_clock::now();
  process();
  auto stop = std::chrono::high_resolution_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
  printf("Elapsed time: %lums\n", duration.count() / 1000);
  return 0;
}
#endif