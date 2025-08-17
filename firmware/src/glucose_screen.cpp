#include "UI.h"
#include "glucose_screen.h"
#include "dexcom/Dexcom_follow.h"

RTC_DATA_ATTR int partialUpdates = 0;
const int PARTIAL_UPDATE_LIMIT = 50;
RTC_DATA_ATTR GlucoseScreen prevgs;

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

void DisplayGlucose(GlucoseReading gl, bool glucoseStrikethrough, String warning, bool updateNeeded, int wifiSignalStrength){
    if (uiLastScreen != GLUCOSE || partialUpdates >= PARTIAL_UPDATE_LIMIT || (warning == "" && prevgs.warning != "")){
        // Write the glucose screen to buffer.
        UiFullClear();
        DrawGlucoseBuffer(gl, glucoseStrikethrough, warning, updateNeeded, wifiSignalStrength);
        UiShow();
        partialUpdates = 0;
    }
    else{
        // write the previous screen to memory to tell the display it has already displayed it. (necccicary for partial update)
        DrawGlucoseBuffer(prevgs.gl, prevgs.glucoseStrikethrough, prevgs.warning, prevgs.updateNeeded, prevgs.wifiSignalStrength);
        UiWriteToMem();

        // Clear the area where the glucose text was displayed to be overwriten.
        ClearGlucoseBuffer(prevgs.gl, prevgs.glucoseStrikethrough, prevgs.warning, prevgs.updateNeeded);
        // Write the glucose screen to buffer.
        DrawGlucoseBuffer(gl, glucoseStrikethrough, warning, updateNeeded, wifiSignalStrength);
        // Partial update the display, showing the image.
        UiShowPartial();
        // Increment a counter so that it full refreshes every 10 partial.
        partialUpdates ++;
    }
    prevgs = {gl, glucoseStrikethrough, warning, updateNeeded, wifiSignalStrength};
}