#ifndef __UPDATE_MANAGER_H
#define __UPDATE_MANAGER_H

bool CheckForUpdate();
bool UpdateFromUrl(String url);
String GetFirmwareUrl();
extern RTC_DATA_ATTR bool updateNeeded;
extern RTC_DATA_ATTR time_t lastCheckedForUpdates;

typedef struct{
    int major;
    int minor;
    int revision;
} Version;

#endif