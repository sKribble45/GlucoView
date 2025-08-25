#include <Arduino.h>
#include "config/config_manager.h"

#ifndef __BG_DATASOURCE_H
#define __BG_DATASOURCE_H
typedef enum{
    DEXCOM,
    NIGHTSCOUT
} BGDataSource;

typedef struct {
    int minsSinceReading;
    unsigned long timestamp;
    unsigned long tztimestamp;
    double bg;
    double delta;
    int mgdl;
    int mgdlDelta;
    char trendDescription[14];

    bool failed;
} GlucoseReading;

extern GlucoseReading gl;

double MgdlToMmol(int mgdl);

bool BgDataSourceInit(Config config);
bool RetrieveGlDataSource();
GlucoseReading GlDataSource();
void PrintGlucose(GlucoseReading gl);

#endif