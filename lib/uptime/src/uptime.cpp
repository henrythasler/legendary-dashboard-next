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

#ifdef ARDUINO
void Uptime::applyLocale(String *input)
{
    input->replace("Monday", "Montag");
    input->replace("Tuesday", "Dienstag");
    input->replace("Wednesday", "Mittwoch");
    input->replace("Thursday", "Donnerstag");
    input->replace("Friday", "Freitag");
    input->replace("Saturday", "Samstag");
    input->replace("Sunday", "Sonntag");

    input->replace("January", "Januar");
    input->replace("February", "Februar");
    input->replace("March", "März");
    input->replace("May", "Mai");
    input->replace("June", "Juni");
    input->replace("July", "Juli");
    input->replace("October", "Oktober");
    input->replace("December", "Dezember");
}
#endif