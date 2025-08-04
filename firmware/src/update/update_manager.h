#ifndef __UPDATE_MANAGER_H
#define __UPDATE_MANAGER_H

bool CheckForUpdates();
extern RTC_DATA_ATTR bool updateNeeded;
extern RTC_DATA_ATTR time_t lastCheckedForUpdates;

#define VERSION 1.1

#endif