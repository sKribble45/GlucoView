
#include "epaper/DEV_Config.h"
#include "epaper/EPD.h"
#include "epaper/GUI_Paint.h"
#include <stdlib.h>
#include <Arduino.h>

#ifndef __UI_H
#define __UI_H

void DispInit();
void UiFullClear();
void UiShow();
void UiShowPartial();
void UiWriteToMem();
void UiWiFiConectionPage(String ssid, String password);
void UiWebPageConectionPage(String pageLink);
void UiGlucose(double glucose, double delta, long epoch);
void UiClearGlucose(double glucose, double delta, long epoch);
void UiWarning(const char *message, const char *subtext);
void UiWarningGlucose(const char *message, double bg,const char *subtext);
void UiTextQrCode(int startX, int startY, const char *link);
void UiWifiQrCode(int startX, int startY, String ssid, String password);

typedef enum {
    NONE = 0,
    WARNING = 1,
    GLUCOSE = 2,
    WARNING_GLUCOSE = 3,
    CONFIG = 4
} UiScreen;

extern RTC_DATA_ATTR UiScreen uiLastScreen;
extern UBYTE *MainImage;

#endif