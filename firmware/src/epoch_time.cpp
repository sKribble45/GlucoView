#include <NTPClient.h>
#include <WiFi.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Get the epoch time from an ntp server (returns 0 when an error has occured)
unsigned long GetEpoch(){
    unsigned long currentTime;

    const int forceUpdateLimit = 10;
    int forceUpdates = 0;
    timeClient.begin();
    while(!timeClient.update() && forceUpdates <= forceUpdateLimit) {
        // Try and force the timeclient update.
        timeClient.forceUpdate();
        Serial.println("Timeclient not updated, forcing update.");
        forceUpdates ++;
        delay(1000);
    }
    if (forceUpdates > forceUpdateLimit){
        return 0;
    }
    else{
        currentTime = timeClient.getEpochTime();
    }


    return currentTime;
}