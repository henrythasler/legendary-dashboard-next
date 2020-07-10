#include <Arduino.h>
#include <Wire.h>

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

// Track initialisation
uint32_t initStage = 0;

// run once on startup
void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  initStage++;

  // Setup serial connection for debugging
  Serial.begin(115200U);
  Serial.println();
  Serial.println("[  INIT  ] Begin");
  initStage++;


  Serial.println("[  INIT  ] setup ePaper display");
  delay(100); // wait a bit, before display-class starts writing to serial out

  display.init(115200); // uncomment serial speed definition for debug output
  io.setFrequency(500000L); // set to 500kHz; the default setting (4MHz) could be unreliable with active modem and unshielded wiring
  delay(100);
  initStage++;

  Serial.printf("[  INIT  ] Completed at stage %u\n\n", initStage);
  digitalWrite(LED_BUILTIN, HIGH); // turn on LED to indicate normal operation;

  // Clear Screen
  display.eraseDisplay();
}

// run forever
void loop()
{
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
}
