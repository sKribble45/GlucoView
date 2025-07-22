#include "dexcom/Dexcom_follow.h"

#ifndef __GLUCOSE_SCREEN_H
#define __GLUCOSE_SCREEN_H

typedef struct{
    GlucoseReading gl;
    bool glucoseStrikethrough;
    String warning;
} GlucoseScreen;

void DisplayGlucose(GlucoseReading gl, bool glucoseStrikethrough, String warning, bool updateNeeded);

extern RTC_DATA_ATTR int partialUpdates;
extern RTC_DATA_ATTR GlucoseScreen prevGlScreen;

#endif