#include <Arduino.h>
#include "glucose/bg_datasource.h"
#ifndef __DATASOURCE_DEXCOM_H
#define __DATASOURCE_DEXCOM_H

bool DexcomInit(String username, String password, bool ous);
GlucoseReading GetBgFromDexcom();
extern String dexUsername;
extern String dexPassword;
extern bool dexOus;
RTC_DATA_ATTR extern char sessionID[37];
RTC_DATA_ATTR extern unsigned long lastSessionIdRenewal;


#endif