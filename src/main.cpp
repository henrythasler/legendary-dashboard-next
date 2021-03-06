#include <Arduino.h>
// #include <LittleFS.h>

#include <WiFi.h>
#include <PubSubClient.h>
/**
 * This file must contain a struct "secrets" with the following properties:
 *  const char *wifiSsid = "test" // WiFi AP-Name
 *  const char *wifiPassword = "1234"
 *  const char *ntpServer = "192.168.0.1";
 *  IPAddress mqttServer = IPAddress(192, 168, 0, 1); // MQTT-Broker
 **/
#include <secrets.h>

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Environment sensor includes and defines
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#define BME280_PIN_I2C_SCL (22)
#define BME280_PIN_I2C_SDA (21)
#define BME280_ADDR (0x76)
#define BME280_PIN_VCC (4)
#define PRESSURE_MEASUREMENT_CALIBRATION (6000)
#define SEALEVEL_PRESSURE (1013.25)
#define SEALEVEL_PRESSURE_CALIBRATION (9.65)

TwoWire I2CBME = TwoWire(0); // set up a new Wire-Instance for BME280 Environment Sensor
Adafruit_BME280 bme;         // use I2C
bool environmentSensorAvailable = false;

// Display stuff
#include <Adafruit_I2CDevice.h>
#include <GxEPD2_3C.h>
GxEPD2_3C<GxEPD2_420c, GxEPD2_420c::HEIGHT> display(GxEPD2_420c(/*CS=VSPI_CS0=D5*/ 5, /*DC=D17*/ 17, /*RST=D16*/ 16, /*BUSY=D15*/ 15));
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
Timeseries insideTemp(1400U);
Timeseries insideHum(1400U);
Timeseries pressure(1400U);
Timeseries outsideTemp(1400U);

// uptime calculation
#include <uptime.h>
Uptime uptime;

// charts
#include <chart.h>
Chart chart;

// images & icons
#include <gfx.h>

// Graphic helper functions
#include <graphics.h>
Graphics graphics;

// file system stuff
#include <SPIFFS.h>
#include <persistency.h>
Persistency persistency;
bool filesystemAvailable = true;

// Flow control, basic task scheduler
#define SCHEDULER_MAIN_LOOP_MS (10) // ms
uint32_t counterBase = 0;
uint32_t counter2s = 0;
uint32_t counter300s = 0;
uint32_t counter1h = 0;
uint32_t initStage = 0;

float currentTemperatureCelsius;
float currentHumidityPercent;
float currentPressurePascal;
float currentOutsideTemperatureCelsius;

char byteBuffer[100];
char textBuffer[100];

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("[  MQTT  ] Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  uint32_t i;
  for (i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    byteBuffer[i] = payload[i];
  }
  byteBuffer[i] = 0;
  currentOutsideTemperatureCelsius = atof(byteBuffer);
  uint32_t timestamp = uptime.getSeconds();
  outsideTemp.push(timestamp, currentOutsideTemperatureCelsius);
  Serial.println();
}

// run once on startup
void setup()
{
  // LED output
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  initStage++;

  // Setup serial connection for debugging
  Serial.begin(115200U);
  delay(500);
  Serial.println();
  Serial.println("[  INIT  ] Begin");
  initStage++;

  Serial.printf("[  INIT  ] ChipRevision: 0x%02X    CpuFreq: %uMHz   FlashChipSize: %uKiB   HeapSize: %uKiB   MAC: %s   SdkVersion: %s\n",
                ESP.getChipRevision(),
                ESP.getCpuFreqMHz(),
                ESP.getFlashChipSize() / 1024,
                ESP.getHeapSize() / 1024,
                WiFi.macAddress().c_str(),
                ESP.getSdkVersion());
  initStage++;

  //connect to your local wi-fi network
  Serial.printf("[  INIT  ] Connecting to Wifi '%s'", secrets.wifiSsid);
  WiFi.begin(secrets.wifiSsid, secrets.wifiPassword);

  //check wi-fi is connected to wi-fi network
  int retries = 5;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
    retries--;
    if (retries <= 0)
    {
      ESP.restart();
    }
  }
  Serial.print(" connected!");
  Serial.print(" (IP=");
  Serial.print(WiFi.localIP());
  Serial.println(")");
  initStage++;

  // Clock setup
  Serial.println("[  INIT  ] Clock synchronization");
  configTime(0, 0, secrets.ntpServer);
  delay(200); // wait for ntp-sync

  // set timezone and DST
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0", 1); //"Europe/Berlin"  from: http://www.famschmid.net/timezones.html
  tzset();                                     // Assign the local timezone from setenv

  tm *tm = uptime.getTime();
  Serial.printf("[  INIT  ] Current time is: %02d.%02d.%04d %02d:%02d:%02d\n", tm->tm_mday, tm->tm_mon, tm->tm_year + 1900, tm->tm_hour, tm->tm_min, tm->tm_sec);
  initStage++;

  // Power-On Environment Sensor
  pinMode(BME280_PIN_VCC, OUTPUT);
  digitalWrite(BME280_PIN_VCC, HIGH);
  delay(5); // wait for BME280 to power up. Takes around 2ms.
  initStage++;

  // Initialize Environment Sensor
  if (I2CBME.begin(BME280_PIN_I2C_SDA, BME280_PIN_I2C_SCL, 250000U)) // set I2C-Clock to 250kHz
  {
    initStage++;
    if (bme.begin(BME280_ADDR, &I2CBME)) // use custom Wire-Instance to avoid interference with other libraries.
    {
      initStage++;
      environmentSensorAvailable = true;
      Serial.println("[  INIT  ] found BME280 environment sensor");
    }
    else
    {
      Serial.println("[ ERROR  ] Could not find a BME280 sensor, check wiring!");
    }
  }
  else
  {
    Serial.println("[ ERROR  ] Could not setup I2C Interface!");
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

  Serial.println("[  INIT  ] setup ePaper display");
  delay(100);                // wait a bit, before display-class starts writing to serial out
  display.init(/*115200U*/); // uncomment serial speed definition for debug output
  delay(100);
  initStage++;

  Serial.print("[  INIT  ] Connecting to MQTT-Server... ");
  mqttClient.setServer(secrets.mqttServer, 1883);
  mqttClient.setCallback(mqttCallback);
  Serial.println("ok");
  initStage++;

  Serial.print("[  INIT  ] Mounting file system... ");
  if (SPIFFS.begin(true))
  {
    Serial.println("ok");
    Serial.printf("[  FILE  ] total: %u KiB  available: %u KiB\n", SPIFFS.totalBytes() / 1024, (SPIFFS.totalBytes() - SPIFFS.usedBytes()) / 1024);
  }
  else
  {
    Serial.println("failed");
    Serial.println("[ ERROR  ] An Error has occurred while mounting SPIFFS");
    filesystemAvailable = false;
  }
  initStage++;

  if (filesystemAvailable)
  {
    Serial.println("[  INIT  ] file list");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file)
    {
      Serial.printf("[  FILE  ] %s - %u Bytes\n", file.name(), file.size());
      file = root.openNextFile();
    }
    root.close();
    file.close();

    Serial.println("[  INIT  ] Restoring data...");
    persistency.loadTimeseries(&insideTemp, "/insideTemp.bin");
    persistency.loadTimeseries(&insideHum, "/insideHum.bin");
    persistency.loadTimeseries(&outsideTemp, "/outsideTemp.bin");
    persistency.loadTimeseries(&pressure, "/pressure.bin");

    insideTemp.trim(uptime.getSeconds(), 2 * 24 * 3600);
    insideHum.trim(uptime.getSeconds(), 2 * 24 * 3600);
    pressure.trim(uptime.getSeconds(), 2 * 24 * 3600);
    outsideTemp.trim(uptime.getSeconds(), 2 * 24 * 3600);
    initStage++;
  }

  Serial.printf("[  INIT  ] Completed at stage %u\n\n", initStage);
}

void updateScreen()
{
  display.fillScreen(GxEPD_WHITE);
  display.drawBitmap(0, 0, images.background.color, display.epd2.WIDTH, display.epd2.HEIGHT, COLOR);
  display.drawBitmap(0, 0, images.background.black, display.epd2.WIDTH, display.epd2.HEIGHT, BLACK);

  // WiFi Signal Strength
  int8_t rssi = WiFi.RSSI();
  int8_t rssiPercent = rssi >= -50 ? 100 : (rssi <= -100 ? 0 : 2 * (rssi + 100));

  if (rssiPercent < 1)
    display.drawInvertedBitmap(375, 0, wifi0, 25, 18, BLACK);
  else if (rssiPercent < 25)
    display.drawInvertedBitmap(375, 0, wifi1, 25, 18, BLACK);
  else if (rssiPercent < 50)
    display.drawInvertedBitmap(375, 0, wifi2, 25, 18, BLACK);
  else if (rssiPercent < 75)
    display.drawInvertedBitmap(375, 0, wifi3, 25, 18, BLACK);
  else
    display.drawInvertedBitmap(375, 0, wifi4, 25, 18, BLACK);

  // Time
  tm *tm = uptime.getTime();
  display.setFont(&NotoSans_Bold30pt7b);
  display.setTextColor(COLOR);

  Dimensions dim;
  graphics.getTextBounds(&display, &dim, tm, "%H:%M");
  display.setCursor(200 - dim.width / 2, 51);
  display.print(tm, "%H:%M");

  // Date
  display.setFont(&NotoSans_Bold13pt8b);
  display.setTextColor(BLACK);

  strftime(textBuffer, sizeof(textBuffer), "%EA, %d. %EB %Y", tm);
  String date(textBuffer);

  uptime.applyLocale(&date);
  graphics.getTextBounds(&display, &dim, date.c_str());
  display.setCursor(200 - dim.width / 2, 82);
  display.print(date.c_str());

  // current values
  display.setFont(&NotoSans_Bold20pt7b);
  display.setTextColor(WHITE);

  snprintf(textBuffer, sizeof(textBuffer), "%.1f", currentTemperatureCelsius);
  graphics.getTextBounds(&display, &dim, textBuffer);
  display.setCursor(140 - dim.width, 126);
  display.print(textBuffer);

  snprintf(textBuffer, sizeof(textBuffer), "%.1f", currentOutsideTemperatureCelsius);
  graphics.getTextBounds(&display, &dim, textBuffer);
  display.setCursor(140 - dim.width, 173);
  display.print(textBuffer);

  display.setTextColor(BLACK);

  snprintf(textBuffer, sizeof(textBuffer), "%.0f", currentHumidityPercent);
  graphics.getTextBounds(&display, &dim, textBuffer);
  display.setCursor(140 - dim.width, 227);
  display.print(textBuffer);

  snprintf(textBuffer, sizeof(textBuffer), "%.0f", currentPressurePascal / 100);
  graphics.getTextBounds(&display, &dim, textBuffer);
  display.setCursor(140 - dim.width, 280);
  display.print(textBuffer);

  // Linecharts
  float tempChartMin = min(insideTemp.min, outsideTemp.min) - 1;
  float tempChartMax = max(insideTemp.max, outsideTemp.max) + 1;

  float tMin = uptime.getSeconds() - 2 * 24 * 3600; // min(min(min(insideTemp.data.front().time, outsideTemp.data.front().time), insideHum.data.front().time), pressure.data.front().time);
  float tMax = uptime.getSeconds();

  // Y-Axis Labels
  display.setFont(&Org_01);
  display.setTextColor(BLACK);
  display.setCursor(172, 92 + 6);
  display.printf("%.1f", tempChartMax);
  display.setCursor(172, 92 + 88 - 3);
  display.printf("%.1f", tempChartMin);

  display.setCursor(172, 190 + 6);
  display.printf("%.0f", insideHum.max);
  display.setCursor(172, 190 + 44 - 3);
  display.printf("%.0f", insideHum.min);

  display.setCursor(172, 242 + 6);
  display.printf("%.1f", pressure.max);
  display.setCursor(172, 242 + 44 - 3);
  display.printf("%.1f", pressure.min);

  // Charts
  chart.lineChart(&display, &insideTemp, 170, 92, 230, 88, 2, COLOR, false, false, false, tempChartMin, tempChartMax, false, false, tMin, tMax);
  chart.lineChart(&display, &outsideTemp, 170, 92, 230, 88, 2, BLACK, false, false, false, tempChartMin, tempChartMax, false, false, tMin, tMax);
  chart.lineChart(&display, &insideHum, 170, 190, 230, 44, 2, BLACK, false, true, true, 0, 0, false, false, tMin, tMax);
  chart.lineChart(&display, &pressure, 170, 242, 230, 44, 2, BLACK, false, true, true, 0, 0, false, false, tMin, tMax);

  // Uptime and Memory stats
  display.setFont(&Org_01);
  display.setTextColor(BLACK);
  display.setCursor(0, 298);
  uint32_t upSeconds = uint32_t(esp_timer_get_time() / 1000000LL);
  display.printf("Free: %uK (%uK)  Up: %uh%02um  WiFi: %i%%  Buf: %i%% %i%% %i%% %i%%",
                 ESP.getFreeHeap() / 1024,
                 ESP.getMaxAllocHeap() / 1024,
                 upSeconds / 3600,
                 upSeconds / 60 % 60,
                 rssiPercent,
                 insideTemp.size() * 100 / insideTemp.maxHistoryLength,
                 outsideTemp.size() * 100 / outsideTemp.maxHistoryLength,
                 insideHum.size() * 100 / insideHum.maxHistoryLength,
                 pressure.size() * 100 / pressure.maxHistoryLength);

  display.display(false);
}

void reconnect()
{
  Serial.print("[  MQTT  ] Attempting MQTT connection... ");
  if (mqttClient.connect(WiFi.getHostname()))
  {
    Serial.println("connected");
    mqttClient.subscribe("home/out/temp/value");
  }
  else
  {
    Serial.print("failed, rc=");
    Serial.print(mqttClient.state());
  }
}

// run forever
void loop()
{
  // 100ms Tasks
  if (!(counterBase % (100L / SCHEDULER_MAIN_LOOP_MS)))
  {
    digitalWrite(LED_BUILTIN, HIGH); // regularly turn on LED
    mqttClient.loop();
  }

  // 500ms Tasks
  if (!(counterBase % (500L / SCHEDULER_MAIN_LOOP_MS)))
  {
  }

  // 2s Tasks
  if (!(counterBase % (2000L / SCHEDULER_MAIN_LOOP_MS)))
  {
    // indicate alive
    digitalWrite(LED_BUILTIN, LOW);

    if (counter2s == 1)
    {
      Serial.print("[  DISP  ] Updating... ");
      updateScreen();
      Serial.println("ok");
    }

    counter2s++;
  }

  // 30s Tasks
  if (!(counterBase % (30000L / SCHEDULER_MAIN_LOOP_MS)))
  {
    if (!mqttClient.connected())
    {
      reconnect();
    }

    if (environmentSensorAvailable)
    {
      // read current measurements
      currentTemperatureCelsius = bme.readTemperature();
      currentHumidityPercent = bme.readHumidity();
      currentPressurePascal = bme.readPressure() + PRESSURE_MEASUREMENT_CALIBRATION;

      // update statistics for each measurement
      uint32_t timestamp = uptime.getSeconds();
      insideTemp.push(timestamp, currentTemperatureCelsius);
      insideHum.push(timestamp, currentHumidityPercent);
      pressure.push(timestamp, currentPressurePascal / 100.); // use hPa

      int len = 0;
      len = snprintf(textBuffer, sizeof(textBuffer), "{\"value\": %.1f, \"timestamp\": %u, \"unit\": \"\u00b0C\"}", currentTemperatureCelsius, uptime.getSeconds());
      mqttClient.publish("home/in/temp", textBuffer, len);
      len = snprintf(textBuffer, sizeof(textBuffer), "%.1f", currentTemperatureCelsius);
      mqttClient.publish("home/in/temp/value", textBuffer, len);

      len = snprintf(textBuffer, sizeof(textBuffer), "{\"value\": %.0f, \"timestamp\": %u, \"unit\": \"%%\"}", currentHumidityPercent, uptime.getSeconds());
      mqttClient.publish("home/in/hum", textBuffer, len);
      len = snprintf(textBuffer, sizeof(textBuffer), "%.0f", currentHumidityPercent);
      mqttClient.publish("home/in/hum/value", textBuffer, len);
    }

    // memory state
    Serial.printf("[ STATUS ] Free: %u KiB (%u KiB)  RSSI:%i dBm  In: %u (%u B)  Out: %u (%u B)  Hum: %u (%u B) Press: %u (%u B) Uptime: %" PRIi64 "s\n",
                  ESP.getFreeHeap() / 1024,
                  ESP.getMaxAllocHeap() / 1024,
                  WiFi.RSSI(),
                  insideTemp.size(),
                  sizeof(insideTemp.data) + sizeof(Point) * insideTemp.data.capacity(),
                  outsideTemp.size(),
                  sizeof(outsideTemp.data) + sizeof(Point) * outsideTemp.data.capacity(),
                  insideHum.size(),
                  sizeof(insideHum.data) + sizeof(Point) * insideHum.data.capacity(),
                  pressure.size(),
                  sizeof(pressure.data) + sizeof(Point) * pressure.data.capacity(),
                  (esp_timer_get_time() / 1000000LL));
  }

  // 300s Tasks
  // e-Paper Display MUST not be updated more often than every 180s to ensure lifetime function
  if (!(counterBase % (300000L / SCHEDULER_MAIN_LOOP_MS)))
  {
    if (counter300s > 0) // skip on startup
    {
      // RAM is limited so we cut off the timeseries after x days
      insideTemp.trim(uptime.getSeconds(), 2 * 24 * 3600);
      insideHum.trim(uptime.getSeconds(), 2 * 24 * 3600);
      pressure.trim(uptime.getSeconds(), 2 * 24 * 3600);
      outsideTemp.trim(uptime.getSeconds(), 2 * 24 * 3600);

      // FIXME: Filter high-frequency noise somehow

      // apply compression (Ramer-Douglas-Peucker)
      insideTemp.compact(0.03);
      insideHum.compact(0.2);
      pressure.compact(0.05);
      outsideTemp.compact(0.2);

      if (filesystemAvailable)
      {
        persistency.saveTimeseries(&insideTemp, "/insideTemp.bin");
        persistency.saveTimeseries(&insideHum, "/insideHum.bin");
        persistency.saveTimeseries(&outsideTemp, "/outsideTemp.bin");
        persistency.saveTimeseries(&pressure, "/pressure.bin");
      }

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
