#include <Arduino.h>
// #include <LittleFS.h>

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


// Display stuff
#include <Adafruit_I2CDevice.h>
#include <GxEPD2_3C.h>
GxEPD2_3C<GxEPD2_420c, GxEPD2_420c::HEIGHT> display(GxEPD2_420c(/*CS=VSPI_CS0=D5*/ 5, /*DC=D17*/ 17, /*RST=D16*/ 16, /*BUSY=D15*/ 15));
#define BLACK (0x0000)
#define WHITE (0xFFFF)
#define COLOR (0xF800)
#define HAS_RED_COLOR

// uptime calculation
#include <uptime.h>
Uptime uptime;

// Flow control, basic task scheduler
#define SCHEDULER_MAIN_LOOP_MS (10) // ms
uint32_t counterBase = 0;
uint32_t counter100ms = 0;
uint32_t initStage = 0;

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

  Serial.printf("[  INIT  ] ChipRevision: 0x%02X    CpuFreq: %uMHz   FlashChipSize: %uKiB   HeapSize: %uKiB   SdkVersion: %s\n",
                ESP.getChipRevision(),
                ESP.getCpuFreqMHz(),
                ESP.getFlashChipSize() / 1024,
                ESP.getHeapSize() / 1024,
                ESP.getSdkVersion());
  initStage++;

  Serial.println("[  INIT  ] setup ePaper display");
  delay(100);                // wait a bit, before display-class starts writing to serial out
  display.init(/*115200U*/); // uncomment serial speed definition for debug output
  delay(100);
  initStage++;

  display.clearScreen();
  Serial.printf("[  INIT  ] Completed at stage %u\n\n", initStage);
}

// run forever
void loop()
{
  // 100ms Tasks
  if (!(counterBase % (100L / SCHEDULER_MAIN_LOOP_MS)))
  {
    // indicate alive
    digitalWrite(LED_BUILTIN, counter100ms % 2);
    counter100ms++;
  }

  delay(SCHEDULER_MAIN_LOOP_MS);
  counterBase++;
}
