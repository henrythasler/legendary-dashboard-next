
#ifndef PERSISTENCY_H
#define PERSISTENCY_H

#include <timeseries.h>
#include <SPIFFS.h>

using namespace std;

class Persistency
{
private:
public:
    Persistency(void);
    void loadTimeseries(Timeseries *series, const char *filename);
    void saveTimeseries(Timeseries *series, const char *filename);

};
#endif