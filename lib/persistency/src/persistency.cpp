#include <persistency.h>

Persistency::Persistency(void)
{
}

#ifdef ARDUINO
void Persistency::loadTimeseries(Timeseries *series, const char *filename)
{
  File file = SPIFFS.open(filename, FILE_READ);
  if (file && file.size() > 0)
  {
    if ((*series).read(&file))
    {
      Serial.printf("[  FILE  ] File was read '%s' (%u values, %u Bytes)\n", filename, series->size(), file.size());
    }
    else
    {
      Serial.println("[ ERROR  ] File read failed");
    }
    file.close();
  }
  else
  {
    Serial.println("[ ERROR  ] There was an error opening the file for reading");
  }
}

void Persistency::saveTimeseries(Timeseries *series, const char *filename)
{
  File file = SPIFFS.open(filename, FILE_WRITE);
  if (file)
  {
    if ((*series).write(&file))
    {
      Serial.printf("[  FILE  ] File was written '%s' (%u values, %u Bytes)\n", filename, series->size(), file.size());
    }
    else
    {
      Serial.println("[ ERROR  ] File write failed");
    }
    file.close();
  }
  else
  {
    Serial.println("[ ERROR  ] There was an error opening the file for writing");
  }
}
#endif