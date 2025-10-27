#include <WiFi.h>
#include "time.h"

const char* ntpServer = "pool.ntp.org";

// Get the epoch time from an ntp server (returns 0 when an error has occured)
unsigned long GetEpoch(){
    configTime(0, 0, ntpServer);
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)){
        return 0;
    }
    time_t epoch;
    time(&epoch);

    return epoch;
}