#include <Arduino.h>
#include <Wire.h>

// Display stuff
#include <GxEPD.h>
#include <GxGDEW042Z15/GxGDEW042Z15.h> // 4.2" b/w/r
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

GxIO_Class io(SPI, /*CS=D8*/ SS, /*DC=D3*/ 0, /*RST=D4*/ 16);
GxEPD_Class display(io, /*RST=D4*/ 16, /*BUSY=D6*/ 12);
#define BLACK (0x0000)
#define WHITE (0xFFFF)
#define COLOR (0xF800)
#define HAS_RED_COLOR

// Statistics Helper-Class
#include <timeseries.h>

// uptime calculation
#include <uptime.h>
Uptime uptime;

// charts
#include <chart.h>
Chart chart;

// Flow control, basic task scheduler
#define SCHEDULER_MAIN_LOOP_MS (10) // ms
uint32_t counterBase = 0;
uint32_t initStage = 0;

uint32_t uptimeSeconds = 0;

void ICACHE_RAM_ATTR onTimerISR()
{
  uptimeSeconds++;
  timer1_write(312500U); // 1s
}

void updateScreen()
{
  display.fillScreen(WHITE);
  Timeseries dampedCosine(200), mod(200), step(200);
  for (int x = 0; x < 200; x+=random(10))
  {
    dampedCosine.push(x, exp(-float(x)/50.)*cos(float(x)/10.)*30+float(random(10))-5.);
    step.push(x, x<100?float(random(10))-5.:float(random(10))+50.);
  }
  chart.lineChart(&display, &dampedCosine, 5, 5, 190, 140, 1.2, BLACK, true);
  chart.lineChart(&display, &dampedCosine, 205, 5, 190, 140, 1.5, BLACK, true);
  chart.lineChart(&display, &dampedCosine, 5, 155, 190, 140, 2, BLACK, true);
  chart.lineChart(&display, &dampedCosine, 205, 155, 190, 140, 2.5, BLACK, true);
  display.update();
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

  Serial.println("[  INIT  ] setup ePaper display");
  delay(100);                // wait a bit, before display-class starts writing to serial out
  display.init(/*115200U*/); // uncomment serial speed definition for debug output
  io.setFrequency(500000U);  // set to 500kHz; the default setting (4MHz) could be unreliable with active modem and unshielded wiring
  delay(100);
  initStage++;

  //Initialize uptime calculation
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);
  timer1_write(312500U); // 1s
  initStage++;

  Serial.printf("[  INIT  ] Completed at stage %u\n\n", initStage);
  updateScreen();
}

// run forever
void loop()
{
  delay(SCHEDULER_MAIN_LOOP_MS);
  counterBase++;
}
