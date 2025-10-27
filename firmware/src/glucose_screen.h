#include "glucose/bg_datasource.h"
#ifndef __GLUCOSE_SCREEN_H
#define __GLUCOSE_SCREEN_H

typedef struct{
    GlucoseReading gl;
    bool glucoseStrikethrough;
    char warning[10];
    bool updateNeeded;
    int wifiSignalStrength;
} GlucoseScreen;

void DisplayGlucose(GlucoseReading gl);
void DisplayError(String warning, bool strikethrough);
void DisplayError(String warning, bool strikethrough, GlucoseReading gl);
void UpdateGsSignalStrength(int signalStrength);
void UpdateGsNeedUpdate(bool updateNeeded);

extern RTC_DATA_ATTR int partialUpdates;
extern RTC_DATA_ATTR GlucoseScreen prevGs;
extern RTC_DATA_ATTR GlucoseScreen Gs;
extern Config bgDSConfig;
#endif