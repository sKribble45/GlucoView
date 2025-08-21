#include <Arduino.h>
#include "glucose/bg_datasource.h"
#ifndef __DATASOURCE_DEXCOM_H
#define __DATASOURCE_DEXCOM_H

bool DexcomInit(String username, String password, bool ous);
GlucoseReading GetBgFromDexcom();
extern String dexUsername;
extern String dexPassword;
extern bool dexOus;
extern String dexSessionID;

#endif