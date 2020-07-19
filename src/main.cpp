#include <Arduino.h>
// #include <LittleFS.h>

#include <WiFi.h>
#include <PubSubClient.h>

/**
 * This file must contain the following variables:
 *  const char *ssid = "test" // WiFi AP-Name
 *  const char *password = "1234" // WiFi-password 
 *  IPAddress server(192, 168, 0, 0); // MQTT-Server
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
Timeseries insideTemp(1000U);
Timeseries insideHum(1000U);
Timeseries pressure(1000U);
Timeseries outsideTemp(1000U);

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

float currentTemperatureCelsius;
float currentHumidityPercent;
float currentPressurePascal;
float currentOutsideTemperatureCelsius;

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

  // //connect to your local wi-fi network
  // Serial.printf("[  INIT  ] Connecting to Wifi '%s'", ssid);
  // WiFi.begin(ssid, password);

  // //check wi-fi is connected to wi-fi network
  // int retries = 5;
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   delay(1000);
  //   Serial.print(".");
  //   retries--;
  //   if (retries <= 0)
  //   {
  //     ESP.restart();
  //   }
  // }
  // Serial.print(" connected!");
  // Serial.print(" (IP=");
  // Serial.print(WiFi.localIP());
  // Serial.println(")");

  // Power-On Environment Sensor
  pinMode(BME280_PIN_VCC, OUTPUT);
  digitalWrite(BME280_PIN_VCC, HIGH);
  delay(5); // wait for BMW280 to power up. Takes around 2ms.
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
  delay(100);            // wait a bit, before display-class starts writing to serial out
  display.init(115200U); // uncomment serial speed definition for debug output
  // io.setFrequency(500000U);  // set to 500kHz; the default setting (4MHz) could be unreliable with active modem and unshielded wiring
  delay(100);
  initStage++;

  // Serial.println("[  INIT  ] Connecting to MQTT-Server...");
  // mqttClient.setServer(server, 1883);
  // mqttClient.setCallback(callback);
  // initStage++;

  Serial.printf("[  INIT  ] Completed at stage %u\n\n", initStage);
}

void updateScreen()
{
  display.fillScreen(GxEPD_WHITE);
  display.drawBitmap(0, 0, images.background.color, display.epd2.WIDTH, display.epd2.HEIGHT, COLOR);
  display.drawBitmap(0, 0, images.background.black, display.epd2.WIDTH, display.epd2.HEIGHT, BLACK);

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
  // display.printf("%.1f", insideTemp.max);
  // display.setCursor(2, 254);
  // display.printf("%.1f", insideTemp.min);

  // display.setCursor(135, 165);
  // display.printf("%.0f", insideHum.max);
  // display.setCursor(135, 254);
  // display.printf("%.0f", insideHum.min);

  // display.setCursor(268, 165);
  // display.printf("%.1f", pressure.max);
  // display.setCursor(268, 254);
  // display.printf("%.1f", pressure.min);

  float tempChartMin = min(insideTemp.min, outsideTemp.min) - 2;
  float tempChartMax = max(insideTemp.max, outsideTemp.max) + 2;

  // Charts
  chart.lineChart(&display, &insideTemp, 170, 92, 230, 88, 1.8, COLOR, false, false, false, tempChartMin, tempChartMax);
  chart.lineChart(&display, &outsideTemp, 170, 92, 230, 88, 1.8, BLACK, false, false, false, tempChartMin, tempChartMax);
  chart.lineChart(&display, &insideHum, 170, 190, 230, 44, 1.8, BLACK);
  chart.lineChart(&display, &pressure, 170, 242, 230, 44, 1.8, BLACK);

  // Uptime and Memory stats
  display.setFont(&Org_01);
  display.setTextColor(BLACK);
  display.setCursor(0, 298);
  display.printf("Free: %uK (%uK)  Temp: %u (%uB)  Hum: %u (%uB) Press: %u (%uB) Up: %" PRIi64 "s",
                 ESP.getFreeHeap() / 1024,
                 ESP.getMaxAllocHeap() / 1024,
                 insideTemp.size(),
                 sizeof(insideTemp.data) + sizeof(Point) * insideTemp.data.capacity(),
                 insideHum.size(),
                 sizeof(insideHum.data) + sizeof(Point) * insideHum.data.capacity(),
                 pressure.size(),
                 sizeof(pressure.data) + sizeof(Point) * pressure.data.capacity(),
                 (esp_timer_get_time() / 1000000LL));

  // display.display(false);
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
  }

  // 30s Tasks
  if (!(counterBase % (30000L / SCHEDULER_MAIN_LOOP_MS)))
  {
    // if (!mqttClient.connected())
    // {
    //   reconnect();
    // }

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
    }

    // memory state
    Serial.printf("[ STATUS ] Free: %u KiB (%u KiB)  In: %u (%u B)  Out: %u (%u B)  Hum: %u (%u B) Press: %u (%u B) Uptime: %" PRIi64 "s\n",
                  ESP.getFreeHeap() / 1024,
                  ESP.getMaxAllocHeap() / 1024,
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
    if (counter300s > 0) // don't trim/compact on startup
    {
      // RAM is limited so we cut off the timeseries after x days
      insideTemp.trim(uptime.getSeconds(), 2 * 24 * 3600);
      insideHum.trim(uptime.getSeconds(), 2 * 24 * 3600);
      pressure.trim(uptime.getSeconds(), 2 * 24 * 3600);
      outsideTemp.trim(uptime.getSeconds(), 2 * 24 * 3600);

      // FIXME: Filter high-frequency noise somehow

      // apply compression (Ramer-Douglas-Peucker)
      insideTemp.compact(0.2);
      insideHum.compact(0.2);
      pressure.compact(0.1);
      outsideTemp.compact(0.2);
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
