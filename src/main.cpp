#include <Arduino.h>

// file system stuff
#include "SPIFFS.h"
bool filesystemAvailable = true;

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
    SPIFFS.remove("/")
  }

  Serial.printf("[  INIT  ] Completed at stage %u\n\n", initStage);
}

// run forever
void loop()
{
  digitalWrite(LED_BUILTIN, HIGH);
  delay(250);
  digitalWrite(LED_BUILTIN, LOW);
  delay(250);
}
