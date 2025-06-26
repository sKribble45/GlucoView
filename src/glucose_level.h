
#ifndef __GLUCOSE_LEVEL_H
#define __GLUCOSE_LEVEL_H

typedef struct
{
    double bg;
    double delta;
    unsigned long timestamp;
    unsigned long tztimestamp;
} GlucoseLevel;

typedef struct{
    String bg;
    String delta;
    String time;
} GlucoseLevelString;

#endif