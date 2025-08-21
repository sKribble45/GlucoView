#include <Arduino.h>
#include "glucose/bg_datasource.h"
#ifndef __DATASOURCE_NIGHTSCOUT_H
#define __DATASOURCE_NIGHTSCOUT_H

void NightscoutInit(String URL, String secret);
GlucoseReading GetBgFromNightscout();
extern String nsURL;
extern String hashedNsSecret;
#endif