#include "UI.h"
#include "glucose_screen.h"
#include "dexcom/Dexcom_follow.h"

RTC_DATA_ATTR int partialUpdates = 0;
RTC_DATA_ATTR GlucoseScreen prevGlScreen;

void DrawGlucoseBuffer(GlucoseReading gl, bool glucoseStrikethrough, String warning, bool updateNeeded){
    UiGlucose(gl);
    if(glucoseStrikethrough){UiGlucoseStrikethrough();}
    if(warning){UiGlucoseWarning(warning);}
    if(updateNeeded){UiUpdateSymbol();}
}
void ClearGlucoseBuffer(GlucoseReading gl, bool glucoseStrikethrough, String warning, bool updateNeeded){
    UiClearGlucose(gl);
    if(glucoseStrikethrough){UiGlucoseClearStrikethrough();}
    if(warning){UiGlucoseClearWarning(warning);}
    if(updateNeeded){UiClearUpdateSymbol();}
}

void DisplayGlucose(GlucoseReading gl, bool glucoseStrikethrough, String warning, bool updateNeeded){
    if (uiLastScreen != GLUCOSE || partialUpdates >= 10){
        // Write the glucose screen to buffer.
        DrawGlucoseBuffer(gl, glucoseStrikethrough, warning, updateNeeded);
        UiShow();
        partialUpdates = 0;
    }
    else{
        // write the previous screen to memory to tell the display it has already displayed it. (necccicary for partial update)
        DrawGlucoseBuffer(prevGlScreen.gl, prevGlScreen.glucoseStrikethrough, prevGlScreen.warning, updateNeeded);
        UiWriteToMem();

        // Clear the area where the glucose text was displayed to be overwriten.
        ClearGlucoseBuffer(prevGlScreen.gl, prevGlScreen.glucoseStrikethrough, prevGlScreen.warning, updateNeeded);
        // Write the glucose screen to buffer.
        DrawGlucoseBuffer(gl, glucoseStrikethrough, warning, updateNeeded);
        // Partial update the display, showing the image.
        UiShowPartial();
        // Increment a counter so that it full refreshes every 10 partial.
        partialUpdates ++;
    }
    prevGlScreen = {gl, glucoseStrikethrough, warning};
}