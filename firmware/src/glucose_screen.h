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

void DisplayGlucose(GlucoseReading gl, bool glucoseStrikethrough, String warning, bool updateNeeded, int wifiSignalStrength);

extern RTC_DATA_ATTR int partialUpdates;
extern RTC_DATA_ATTR GlucoseScreen prevgs;

#endif