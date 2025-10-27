#include "UI.h"
#include "glucose_screen.h"
#include "glucose/bg_datasource.h"
#include "wifi_manager.h"

RTC_DATA_ATTR int partialUpdates = 0;
const int PARTIAL_UPDATE_LIMIT = 50;
RTC_DATA_ATTR GlucoseScreen prevGs;
RTC_DATA_ATTR GlucoseScreen Gs;

void DrawGlucoseBuffer(GlucoseReading gl, bool glucoseStrikethrough, String warning, bool updateNeeded, int wifiSignalStrength){
    UiGlucose(gl);
    if(glucoseStrikethrough){UiGlucoseStrikethrough();}
    if(warning){UiGlucoseWarning(warning);}
    if(updateNeeded){UiUpdateIcon();}
    if (GetBooleanValue("wifi-icon", UiConfig)){UiWiFiIcon(wifiSignalStrength);}
}
void ClearGlucoseBuffer(GlucoseReading gl, bool glucoseStrikethrough, String warning, bool updateNeeded){
    UiClearGlucose(gl);
    if(glucoseStrikethrough){UiGlucoseClearStrikethrough();}
    if(warning){UiGlucoseClearWarning(warning);}
    if(updateNeeded){UiClearUpdateIcon();}
    if (GetBooleanValue("wifi-icon", UiConfig)){UiClearWiFiIcon();}
}

void DisplayGlScreen(GlucoseReading gl, bool glucoseStrikethrough, String warning, bool updateNeeded, int wifiSignalStrength){
    if (uiLastScreen != GLUCOSE || partialUpdates >= PARTIAL_UPDATE_LIMIT){
        // Write the glucose screen to buffer.
        UiFullClear();
        DrawGlucoseBuffer(gl, glucoseStrikethrough, warning, updateNeeded, wifiSignalStrength);
        UiShow();
        partialUpdates = 0;
    }
    else{
        // write the previous screen to memory to tell the display it has already displayed it. (necccicary for partial update)
        DrawGlucoseBuffer(prevGs.gl, prevGs.glucoseStrikethrough, prevGs.warning, prevGs.updateNeeded, prevGs.wifiSignalStrength);
        UiWriteToMem();

        // Clear the area where the glucose text was displayed to be overwriten.
        ClearGlucoseBuffer(prevGs.gl, prevGs.glucoseStrikethrough, prevGs.warning, prevGs.updateNeeded);
        // Write the glucose screen to buffer.
        DrawGlucoseBuffer(gl, glucoseStrikethrough, warning, updateNeeded, wifiSignalStrength);
        // Partial update the display, showing the image.
        UiShowPartial();
        // Increment a counter so that it full refreshes every 10 partial.
        partialUpdates ++;
    }

    warning.toCharArray(prevGs.warning, sizeof(prevGs.warning));
    prevGs.gl = gl;
    prevGs.glucoseStrikethrough = glucoseStrikethrough;
    prevGs.updateNeeded = updateNeeded;
    prevGs.wifiSignalStrength = wifiSignalStrength;
}

void DisplayGlucose(GlucoseReading gl){
    DisplayGlScreen(gl, false, "", Gs.updateNeeded, Gs.wifiSignalStrength);
}


void DisplayError(String warning, bool strikethrough){
    DisplayGlScreen(prevGs.gl, strikethrough, warning, Gs.updateNeeded, Gs.wifiSignalStrength);
}
void DisplayError(String warning, bool strikethrough, GlucoseReading gl){
    DisplayGlScreen(gl, strikethrough, warning, Gs.updateNeeded, Gs.wifiSignalStrength);
}

void UpdateGsSignalStrength(int signalStrength){
    Gs.wifiSignalStrength = signalStrength;
}

void UpdateGsNeedUpdate(bool updateNeeded){
    Gs.updateNeeded = updateNeeded;
}
