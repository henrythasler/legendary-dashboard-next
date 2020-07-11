#include <uptime.h>

Uptime::Uptime(void)
{
}

uint32_t Uptime::getSeconds(void)
{
    timeval curTime;
    gettimeofday(&curTime, NULL);
    return curTime.tv_sec;
};

uint32_t Uptime::getMicros()
{
    timeval curTime;
    gettimeofday(&curTime, NULL);
    return curTime.tv_usec;
};

bool Uptime::setTime(tm time)
{
    time_t t = mktime(&time);
    struct timeval now = {.tv_sec = t};
    settimeofday(&now, NULL);
    return true;
}

tm *Uptime::getTime(void)
{
    time_t nowtime = getSeconds();
    return localtime(&nowtime);
}

bool Uptime::parseModemTime(const char *modemTime)
{
    tm tm;
    string timeStr(modemTime);
    // example: "0,2020/07/05,06:21:02"
    if (timeStr.length() >= 21)
    {
        // try
        {
            tm.tm_year = atoi(timeStr.substr(2, 4).c_str()) - 1900;
            tm.tm_mon = atoi(timeStr.substr(7, 2).c_str());
            tm.tm_mday = atoi(timeStr.substr(10, 2).c_str());
            tm.tm_hour = atoi(timeStr.substr(13, 2).c_str());
            tm.tm_min = atoi(timeStr.substr(16, 2).c_str());
            tm.tm_sec = atoi(timeStr.substr(19, 2).c_str());
            return setTime(tm);
        }
//         catch (const std::exception &e)
//         {
// #ifdef ARDUINO
//             Serial.printf("[ ERROR ] Uptime::parseModemTime(\"%s\"): %s\n", modemTime, e.what());
// #else
//             printf("[ ERROR ] Uptime::parseModemTime(\"%s\"): %s\n", modemTime, e.what());
// #endif
//             return false;
//         }
    }
    return false;
}