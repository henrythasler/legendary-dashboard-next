#include <Arduino.h>
#include <Wire.h>

// Environment sensor includes and defines
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define BME280_ADDR (0x76)
#define BME280_PIN_VCC (2)
#define PRESSURE_MEASUREMENT_CALIBRATION (6000)
#define SEALEVEL_PRESSURE (1013.25)
#define SEALEVEL_PRESSURE_CALIBRATION (9.65)

Adafruit_BME280 bme; // use I2C
bool environmentSensorAvailable = false;

// Display stuff
#include <GxEPD.h>
#include <GxGDEW042Z15/GxGDEW042Z15.h> // 4.2" b/w/r
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

// #define PIN_SPI_SS   (15)
// #define PIN_SPI_MOSI (13)
// #define PIN_SPI_MISO (12)
// #define PIN_SPI_SCK  (14)

// MISO=D6 (GPIO12)

// SS=D8 (GPIO15) = CS
// MOSI=D7 (GPIO13) = DIN
// SCK=D5 (GPIO14) = CLK
// D0 (GPIO16) = RST
// D3 (GPIO0) = DC
// D6 (GPIO12) = BUSY

GxIO_Class io(SPI, /*CS=D8*/ SS, /*DC=D3*/ 0, /*RST=D4*/ 16);
GxEPD_Class display(io, /*RST=D4*/ 16, /*BUSY=D6*/ 12);
#define BLACK (0x0000)
#define WHITE (0xFFFF)
#define COLOR (0xF800)
#define HAS_RED_COLOR

#include <Fonts/Org_01.h>

// Custom 8-bit Fonts including character codes 128-255 (e.g. öäü)
#include <FreeSans7pt8b.h>
#include <FreeSansBold7pt8b.h>
#include <FreeSans9pt8b.h>
#include <FreeSansBold9pt8b.h>
#include <FreeSans12pt8b.h>
#include <FreeSansBold12pt8b.h>
#include <FreeSansBold14pt8b.h>
#include <LiberationSansNarrow9pt8b.h>
#include <LiberationSansNarrowBold9pt8b.h>

// Statistics Helper-Class
#include <timeseries.h>
Timeseries tempStats(5000U);
Timeseries humStats(5000U);
Timeseries pressStats(5000U);

// uptime calculation
#include <uptime.h>
Uptime uptime;

// charts
#include <chart.h>
Chart chart;

// Flow control, basic task scheduler
#define SCHEDULER_MAIN_LOOP_MS (10) // ms
uint32_t counterBase = 0;
uint32_t counter300s = 0;
uint32_t counter1h = 0;
uint32_t initStage = 0;

uint32_t uptimeSeconds = 0;

float currentTemperatureCelsius;
float currentHumidityPercent;
float currentPressurePascal;

void updateScreen()
{
  display.fillScreen(WHITE);

  // current values
  display.setFont(&FreeSansBold14pt8b);
  display.setTextColor(BLACK);
  display.setCursor(35, 282);
  display.printf("%.1f\xb0"
                 "C",
                 currentTemperatureCelsius);
  display.setCursor(180, 282);
  display.printf("%.0f%%", currentHumidityPercent);
  display.setCursor(290, 282);
  display.printf("%.0f hPa", currentPressurePascal / 100);

  // Linecharts
  // Y-Axis Labels
  display.setFont(&Org_01);
  display.setTextColor(BLACK);
  display.setCursor(2, 165);
  display.printf("%.1f", tempStats.max);
  display.setCursor(2, 254);
  display.printf("%.1f", tempStats.min);

  display.setCursor(135, 165);
  display.printf("%.0f", humStats.max);
  display.setCursor(135, 254);
  display.printf("%.0f", humStats.min);

  display.setCursor(268, 165);
  display.printf("%.1f", pressStats.max);
  display.setCursor(268, 254);
  display.printf("%.1f", pressStats.min);

  // Frame
  display.drawFastHLine(0, 159, 400, BLACK);
  display.drawFastHLine(0, 256, 400, BLACK);
  display.drawFastVLine(133, 159, 97, BLACK);
  display.drawFastVLine(266, 159, 97, BLACK);

  // Charts
  chart.lineChart(&display, &tempStats, 0, 160, 130, 95, BLACK);
  chart.lineChart(&display, &humStats, 135, 160, 130, 95, BLACK);
  chart.lineChart(&display, &pressStats, 270, 160, 130, 95, BLACK);

  // Uptime and Memory stats
  display.setFont(&Org_01);
  display.fillRect(0, 292, 400, 299, BLACK);
  display.setTextColor(WHITE);
  display.setCursor(0, 298);
  display.printf("Free: %uK (%uK)  Temp: %u (%uB)  Hum: %u (%uB) Press: %u (%uB) Up: %us",
                 ESP.getFreeHeap() / 1024,
                 ESP.getMaxFreeBlockSize() / 1024,
                 tempStats.size(),
                 sizeof(tempStats.data) + sizeof(Point) * tempStats.data.capacity(),
                 humStats.size(),
                 sizeof(humStats.data) + sizeof(Point) * humStats.data.capacity(),
                 pressStats.size(),
                 sizeof(pressStats.data) + sizeof(Point) * pressStats.data.capacity(),
                 uptimeSeconds);

  display.update();
}

void ICACHE_RAM_ATTR onTimerISR()
{
  uptimeSeconds++;
  timer1_write(312500U); //12us
}

// run once on startup
void setup()
{
  pinMode(BME280_PIN_VCC, OUTPUT);
  digitalWrite(BME280_PIN_VCC, HIGH);
  delay(5); // wait for BMW280 to power up. Takes around 2ms.
  initStage++;

  // Setup serial connection for debugging
  Serial.begin(115200U);
  Serial.println();
  Serial.println("[  INIT  ] Begin");
  initStage++;

  // Initialize Environment Sensor
  if (bme.begin(BME280_ADDR, &Wire)) // use custom Wire-Instance to avoid interference with other libraries.
  {
    initStage++;
    environmentSensorAvailable = true;
    Serial.println("[  INIT  ] found BME280 environment sensor");
  }

  if (environmentSensorAvailable)
  {
    initStage++;
    // Setup Environment Sensor
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,     /* Operating Mode. */
                    Adafruit_BME280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BME280::SAMPLING_X16,    /* Hum. oversampling */
                    Adafruit_BME280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BME280::FILTER_X16,      /* Filtering. */
                    Adafruit_BME280::STANDBY_MS_500); /* Standby time. */
    Serial.println("[  INIT  ] BME280 setup done");
  }
  else
  {
    Serial.println("[ ERROR  ] Could not find a BME280 sensor, check wiring!");
  }
  initStage++;

  Serial.println("[  INIT  ] setup ePaper display");
  delay(100);               // wait a bit, before display-class starts writing to serial out
  display.init(115200U);     // uncomment serial speed definition for debug output
  io.setFrequency(500000U); // set to 500kHz; the default setting (4MHz) could be unreliable with active modem and unshielded wiring
  delay(100);
  initStage++;

  //Initialize Ticker every 1s
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);
  timer1_write(312500U); // 1s

  Serial.printf("[  INIT  ] Completed at stage %u\n\n", initStage);
}

// run forever
void loop()
{
  // 100ms Tasks
  if (!(counterBase % (100L / SCHEDULER_MAIN_LOOP_MS)))
  {
  }

  // 500ms Tasks
  if (!(counterBase % (500L / SCHEDULER_MAIN_LOOP_MS)))
  {
  }

  // 2s Tasks
  if (!(counterBase % (2000L / SCHEDULER_MAIN_LOOP_MS)))
  {
  }

  // 30s Tasks
  if (!(counterBase % (30000L / SCHEDULER_MAIN_LOOP_MS)))
  {
    if (environmentSensorAvailable)
    {
      // read current measurements
      currentTemperatureCelsius = bme.readTemperature();
      currentHumidityPercent = bme.readHumidity();
      currentPressurePascal = bme.readPressure() + PRESSURE_MEASUREMENT_CALIBRATION;

      // Serial.printf("Temp=%.1f°C  Hum=%.1f%%  Press=%.1fhPa\n", currentTemperatureCelsius, currentHumidityPercent, currentPressurePascal / 100.);
      // memory state
      Serial.printf("[ MEMORY ] Free: %u KiB (%u KiB)  Temp: %u (%u B)  Hum: %u (%u B) Press: %u (%u B) Uptime: %us\n",
                    ESP.getFreeHeap() / 1024,
                    ESP.getMaxFreeBlockSize() / 1024,
                    tempStats.size(),
                    sizeof(tempStats.data) + sizeof(Point) * tempStats.data.capacity(),
                    humStats.size(),
                    sizeof(humStats.data) + sizeof(Point) * humStats.data.capacity(),
                    pressStats.size(),
                    sizeof(pressStats.data) + sizeof(Point) * pressStats.data.capacity(),
                    uptimeSeconds);

      // update statistics for each measurement
      uint32_t timestamp = uptime.getSeconds();
      tempStats.push(timestamp, currentTemperatureCelsius);
      humStats.push(timestamp, currentHumidityPercent);
      pressStats.push(timestamp, currentPressurePascal / 100.); // use hPa
    }
  }

  // 300s Tasks
  // e-Paper Display MUST not be updated more often than every 180s to ensure lifetime function
  if (!(counterBase % (300000L / SCHEDULER_MAIN_LOOP_MS)))
  {
    if (counter300s > 0) // don't trim/compact on startup
    {
      // RAM is limited so we cut off the timeseries after x days
      tempStats.trim(uptime.getSeconds(), 7 * 24 * 3600);
      humStats.trim(uptime.getSeconds(), 7 * 24 * 3600);
      pressStats.trim(uptime.getSeconds(), 7 * 24 * 3600);

      // FIXME: Filter high-frequency noise somehow

      // apply compression (Ramer-Douglas-Peucker)
      tempStats.compact(0.03);
      humStats.compact(0.2);
      pressStats.compact(0.05);
    }

    if (true)
    {
      Serial.print("[  DISP  ] Updating... ");
      updateScreen();
      Serial.println("ok");
    }

    counter300s++;
  }

  // 1h Tasks
  if (!(counterBase % (3600000L / SCHEDULER_MAIN_LOOP_MS)))
  {
    counter1h++;
  }

  delay(SCHEDULER_MAIN_LOOP_MS);
  counterBase++;
}
