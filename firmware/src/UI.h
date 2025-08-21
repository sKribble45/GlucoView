
#include "epaper/DEV_Config.h"
#include "epaper/EPD.h"
#include "epaper/GUI_Paint.h"
#include <stdlib.h>
#include <Arduino.h>
#include "glucose/bg_datasource.h"
#include "config/config_manager.h"

#ifndef __UI_H
#define __UI_H

void DispInit();
void UiFullClear();
void UiShow();
void UiShowPartial();
void UiWriteToMem();
void UiWiFiConectionPage(String ssid, String password);
void UiWebPageConectionPage(String pageLink);
void UiGlucose(GlucoseReading gl);
void UiClearGlucose(GlucoseReading gl);
void UiGlucoseStrikethrough();
void UiGlucoseClearStrikethrough();
void UiGlucoseWarning(String warning);
void UiGlucoseClearWarning(String warning);
void UiUpdateIcon();
void UiClearUpdateIcon();
void UiWiFiIcon(int value);
void UiClearWiFiIcon();
void UiClearText(bool centered, int Xstart, int Ystart, const char * pString, sFONT* Font);
void UiWarning(const char *message, const char *subtext);
void UiTextQrCode(int startX, int startY, const char *link);
void UiWifiQrCode(int startX, int startY, String ssid, String password);
void UiUpdateMode();
void UiSetupScreen();
void UiSetupFinish();
void UiInitConfig(Config config);

extern Config UiConfig;

typedef enum {
    NONE = 0,
    WARNING = 1,
    GLUCOSE = 2,
    WARNING_GLUCOSE = 3,
    CONFIG = 4,
    UPDATE = 5,
    SETUP = 6
} UiScreen;

extern RTC_DATA_ATTR UiScreen uiLastScreen;
extern UBYTE *MainImage;

#endif