#include "UI.h"
#include "glucose_screen.h"
#include "dexcom/Dexcom_follow.h"

RTC_DATA_ATTR int partialUpdates = 0;
RTC_DATA_ATTR GlucoseScreen prevGlScreen;

void DrawGlucoseBuffer(GlucoseReading gl, bool glucoseStrikethrough, String warning){
    UiGlucose(gl);
    if(glucoseStrikethrough){UiGlucoseStrikethrough();}
    if(warning){UiGlucoseWarning(warning);}
}
void ClearGlucoseBuffer(GlucoseReading gl, bool glucoseStrikethrough, String warning){
    UiClearGlucose(gl);
    if(glucoseStrikethrough){UiGlucoseClearStrikethrough();}
    if(warning){UiGlucoseClearWarning(warning);}
}

void DisplayGlucose(GlucoseReading gl, bool glucoseStrikethrough, String warning){
    if (uiLastScreen != GLUCOSE || partialUpdates >= 10){
        // Write the glucose screen to buffer.
        DrawGlucoseBuffer(gl, glucoseStrikethrough, warning);
        UiShow();
        partialUpdates = 0;
    }
    else{
        // write the previous screen to memory to tell the display it has already displayed it. (necccicary for partial update)
        DrawGlucoseBuffer(prevGlScreen.gl, prevGlScreen.glucoseStrikethrough, prevGlScreen.warning);
        UiWriteToMem();

        // Clear the area where the glucose text was displayed to be overwriten.
        ClearGlucoseBuffer(prevGlScreen.gl, prevGlScreen.glucoseStrikethrough, prevGlScreen.warning);
        // Write the glucose screen to buffer.
        DrawGlucoseBuffer(gl, glucoseStrikethrough, warning);
        // Partial update the display, showing the image.
        UiShowPartial();
        // Increment a counter so that it full refreshes every 10 partial.
        partialUpdates ++;
    }
    prevGlScreen = {gl, glucoseStrikethrough, warning};
}