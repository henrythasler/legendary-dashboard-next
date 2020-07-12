#include <Arduino.h>
#include <Wire.h>
#include <LittleFS.h>

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <secrets.h>

WiFiClient wifiClient;
PubSubClient client(wifiClient);

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
#include <NotoSansBold13pt8b.h>

// Only Numbers and some signs
#include <NotoSansBold20ptNum.h>
#include <NotoSansBold30ptNum.h>

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

#include <gfx.h>

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
float currentOutsideTemperatureCelsius;

void ICACHE_RAM_ATTR onTimerISR()
{
  uptimeSeconds++;
  timer1_write(312500U); // 1s
}

char message_buff[100];
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("[  MQTT  ] Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  uint32_t i;
  for (i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    message_buff[i] = payload[i];
  }
  message_buff[i] = 0;
  currentOutsideTemperatureCelsius = atof(message_buff);
  Serial.println();
}

// run once on startup
void setup()
{
  // Setup serial connection for debugging
  Serial.begin(115200U);
  delay(500);
  Serial.println();
  Serial.println("[  INIT  ] Begin");
  initStage++;

  //connect to your local wi-fi network
  Serial.printf("[  INIT  ] Connecting to Wifi '%s'", ssid);
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.print(" connected!");
  Serial.print(" (IP=");
  Serial.print(WiFi.localIP());
  Serial.println(")");

  // Power-On Environment Sensor
  pinMode(BME280_PIN_VCC, OUTPUT);
  digitalWrite(BME280_PIN_VCC, HIGH);
  delay(5); // wait for BMW280 to power up. Takes around 2ms.
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
                    Adafruit_BME280::SAMPLING_X16,    /* Temp. oversampling */
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
  delay(100);                // wait a bit, before display-class starts writing to serial out
  display.init(/*115200U*/); // uncomment serial speed definition for debug output
  io.setFrequency(500000U);  // set to 500kHz; the default setting (4MHz) could be unreliable with active modem and unshielded wiring
  delay(100);
  initStage++;

  //Initialize File System
  if (LittleFS.begin())
  {
    FSInfo fsInfo;
    LittleFS.info(fsInfo);
    Serial.printf("[  INIT  ] LittleFS initialized (Total=%u KiB  Free=%u KiB  maxPathLength=%u)\n",
                  fsInfo.totalBytes / 1024,
                  (fsInfo.totalBytes - fsInfo.usedBytes) / 1024,
                  fsInfo.maxPathLength);

    // Open dir folder
    Dir dir = LittleFS.openDir("/");
    // Cycle all the content
    while (dir.next())
    {
      // get filename
      Serial.print("[  INIT  ] ");
      Serial.print(dir.fileName());
      Serial.print(" - ");
      // If element have a size display It else write 0
      if (dir.fileSize())
      {
        File f = dir.openFile("r");
        Serial.println(f.size());
        f.close();
      }
      else
      {
        Serial.println("0");
      }
    }
  }
  else
  {
    Serial.println("[  INIT  ] LittleFS initialization failed");
  }
  initStage++;

  //Initialize uptime calculation
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);
  timer1_write(312500U); // 1s
  initStage++;

  Serial.println("[  INIT  ] Connecting to MQTT-Server...");
  client.setServer(server, 1883);
  client.setCallback(callback);
  initStage++;

  Serial.printf("[  INIT  ] Completed at stage %u\n\n", initStage);
}

void updateScreen()
{
  display.drawBitmap(images.background.color, 0, 0, 400, 300, COLOR, display.bm_invert);
  display.drawBitmap(images.background.black, 0, 0, 400, 300, BLACK, display.bm_invert | display.bm_transparent);

  // Time
  display.setFont(&NotoSans_Bold30pt7b);
  display.setTextColor(COLOR);
  display.setCursor(128, 51);
  display.printf("13:37");

  // Date
  display.setFont(&NotoSans_Bold13pt8b);
  display.setTextColor(BLACK);
  display.setCursor(3, 82);
  display.printf("Samstag, 11. Dezember 2020");

  // current values
  display.setFont(&NotoSans_Bold20pt7b);
  display.setTextColor(WHITE);
  display.setCursor(48, 126);
  display.printf("%.1f", currentTemperatureCelsius);
  display.setCursor(48, 173);
  display.printf("%.1f", currentOutsideTemperatureCelsius);

  display.setTextColor(BLACK);
  display.setCursor(95, 227);
  display.printf("%.0f", currentHumidityPercent);
  display.setCursor(48, 280);
  display.printf("%.0f", currentPressurePascal / 100);

  // Linecharts
  // Y-Axis Labels
  // display.setFont(&Org_01);
  // display.setTextColor(BLACK);
  // display.setCursor(2, 165);
  // display.printf("%.1f", tempStats.max);
  // display.setCursor(2, 254);
  // display.printf("%.1f", tempStats.min);

  // display.setCursor(135, 165);
  // display.printf("%.0f", humStats.max);
  // display.setCursor(135, 254);
  // display.printf("%.0f", humStats.min);

  // display.setCursor(268, 165);
  // display.printf("%.1f", pressStats.max);
  // display.setCursor(268, 254);
  // display.printf("%.1f", pressStats.min);

  // Charts
  chart.lineChart(&display, &tempStats, 170, 92, 230, 88, 1.5, COLOR);
  chart.lineChart(&display, &humStats, 170, 190, 230, 44, 1.5, BLACK);
  chart.lineChart(&display, &pressStats, 170, 242, 230, 44, 1.5, BLACK);

  // Uptime and Memory stats
  display.setFont(&Org_01);
  display.setTextColor(BLACK);
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

void reconnect()
{
  Serial.print("[  MQTT  ] Attempting MQTT connection... ");
  if (client.connect(WiFi.hostname().c_str()))
  {
    Serial.println("connected");
    client.subscribe("home/out/temp/value");
  }
  else
  {
    Serial.print("failed, rc=");
    Serial.print(client.state());
  }
}

// run forever
void loop()
{
  // 100ms Tasks
  if (!(counterBase % (100L / SCHEDULER_MAIN_LOOP_MS)))
  {
    client.loop();
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
    if (!client.connected())
    {
      reconnect();
      delay(100);
      client.loop();
    }

    if (environmentSensorAvailable)
    {
      // read current measurements
      currentTemperatureCelsius = bme.readTemperature();
      currentHumidityPercent = bme.readHumidity();
      currentPressurePascal = bme.readPressure() + PRESSURE_MEASUREMENT_CALIBRATION;

      // Serial.printf("Temp=%.1f°C  Hum=%.1f%%  Press=%.1fhPa\n", currentTemperatureCelsius, currentHumidityPercent, currentPressurePascal / 100.);
      // memory state
      Serial.printf("[ STATUS ] Free: %u KiB (%u KiB)  Temp: %u (%u B)  Hum: %u (%u B) Press: %u (%u B) Uptime: %us\n",
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
