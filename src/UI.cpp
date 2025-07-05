#include "epaper/DEV_Config.h"
#include "epaper/EPD.h"
#include "epaper/GUI_Paint.h"
#include <stdlib.h>
#include <Arduino.h>
#include <qrcode.h>
#include "UI.h"
#include <ctime>
#include "config/config_manager.h"
#include "dexcom/Dexcom_follow.h"
using namespace std;

UBYTE *MainImage;
RTC_DATA_ATTR UiScreen uiLastScreen = NONE;

void DispInit(){
    DEV_Module_Init();

    // init display
    EPD_2in13_V4_Init();
    //Create a new image cache
    printf("Paint_NewImage\r\n");
    UWORD Imagesize = ((EPD_2in13_V4_WIDTH % 8 == 0)? (EPD_2in13_V4_WIDTH / 8 ): (EPD_2in13_V4_WIDTH / 8 + 1)) * EPD_2in13_V4_HEIGHT;
	if((MainImage = (UBYTE *)malloc(Imagesize)) == NULL) 
	{
		printf("Failed to apply for black memory...\r\n");
        delay(1000);
		ESP.restart();
	}
    Paint_NewImage(MainImage, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE); 
    Paint_Clear(WHITE);
}
void UiFullClear(){
    Paint_SelectImage(MainImage);
    Paint_NewImage(MainImage, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);  	
    Paint_Clear(WHITE);
}
void UiShow(){
    EPD_2in13_V4_Display(MainImage);
}
void UiShowPartial(){
    EPD_2in13_V4_Display_Partial(MainImage);
}
void UiWriteToMem(){
    EPD_2in13_V4_Display_Mem(MainImage);
}


void UiClearCenteredText(int Xstart, int Ystart, const char * pString, sFONT* Font){
    int string_length = strlen(pString);
    int Xpoint = Xstart - ((Font->Width * string_length) / 2);
    int Ypoint = Ystart - (Font->Height / 2);
    Paint_ClearWindows(Xpoint, Ypoint, Xpoint + (Font->Width * string_length), Ypoint + Font->Height, WHITE);
}
struct GlucoseReadingString{
    String bg;
    String delta;
    String time;
};

// Makes the gl into a string version ready for drawing to the screen.
static GlucoseReadingString GetGLChar(GlucoseReading gl){
    UserConfig config;
    LoadConfig(config);
    // Get UTC time from the epoch
    time_t epochTime = gl.tztimestamp;
    struct tm *utc = gmtime(&epochTime);
    
    // Make time string
    int minutes = utc->tm_min;
    int hours = utc->tm_hour;
    // ajust hours if 12h time
    bool AM = false;
    if (config.twelveHourTime){
        if (hours == 12){AM = false;}
        else if (hours > 12){hours -= 12;}
        else {AM = true;}
    }
    string minutesString;
    string hoursString;

    // Statring from a blank string add the 0 at the start if the number of minutes or hours is bellow 10 otherwise the time could look like this: 1:4 instead of this: 01:04
    if (minutes < 10){minutesString += "0";}
    minutesString += to_string(minutes);
    if (hours < 10 && !config.twelveHourTime){hoursString += "0";}
    hoursString += to_string(hours);

    // Create a string with the hour and minute of the timestamp.
    string timestr = hoursString + ":" + minutesString;
    // add the suffix AM or PM to the 12h time.
    if (config.twelveHourTime){
        if(AM){timestr += " AM";}
        else{timestr += " PM";}
    }

    // Convert bg into char
    char bgChar[5];
    sprintf(bgChar, "%.1f", gl.bg);

    // Add a plus to the delta if it is positive.
    char deltaFancy[5];
    if (gl.delta >= 0.0){ sprintf(deltaFancy, "+%.1f", gl.delta); }
    else{ sprintf(deltaFancy, "%.1f", gl.delta); }
    GlucoseReadingString glchr = {bgChar, deltaFancy, timestr.c_str()};

    return glchr;
}

// Draw Glucose screen (includes glucose level, delta and timestamp)
void UiGlucose(GlucoseReading gl){
    GlucoseReadingString glstr = GetGLChar(gl); 

    // Draw bg
    Paint_DrawString_EN(true, (EPD_2in13_V4_HEIGHT / 2) - 25, EPD_2in13_V4_WIDTH / 2, glstr.bg.c_str(), &Font64, BLACK, WHITE);
    // Draw Delta
    Paint_DrawString_EN(true, (EPD_2in13_V4_HEIGHT / 2) + 80, (EPD_2in13_V4_WIDTH / 2) + 25, glstr.delta.c_str(), &Font20, WHITE, BLACK);
    // Draw timestamp
    Paint_DrawString_EN(true, (EPD_2in13_V4_HEIGHT / 2), (EPD_2in13_V4_WIDTH / 2) + 45, glstr.time.c_str(), &Font16, WHITE, BLACK);

    uiLastScreen = GLUCOSE;
}
// Clear Glucose screen (includes glucose level, delta and timestamp) ready for partial update.
void UiClearGlucose(GlucoseReading gl){
    GlucoseReadingString glstr = GetGLChar(gl); 

    // Clear bg
    UiClearCenteredText((EPD_2in13_V4_HEIGHT / 2) - 25, EPD_2in13_V4_WIDTH / 2, glstr.bg.c_str(), &Font64);
    // Clear Delta
    UiClearCenteredText((EPD_2in13_V4_HEIGHT / 2) + 80, (EPD_2in13_V4_WIDTH / 2) + 25, glstr.delta.c_str(), &Font20);
    // Clear timestamp
    UiClearCenteredText((EPD_2in13_V4_HEIGHT / 2), (EPD_2in13_V4_WIDTH / 2) + 45, glstr.time.c_str(), &Font16);
}




// Displays a warning message on screen with subtext.
void UiWarning(const char *message, const char *subtext){
    Paint_DrawString_EN(true, EPD_2in13_V4_HEIGHT / 2, EPD_2in13_V4_WIDTH / 2, message, &Font24, WHITE, BLACK);
    Paint_DrawString_EN(true, EPD_2in13_V4_HEIGHT / 2, (EPD_2in13_V4_WIDTH / 2) + Font24.Height, subtext, &Font8, WHITE, BLACK);
    uiLastScreen = WARNING;
}

// Displays a warning message on screen with a bg and subtext.
void UiWarningGlucose(const char *message, double bg,const char *subtext){
    char bgChar[5];
    sprintf(bgChar, "%.1f", bg);
    Paint_DrawString_EN(true, EPD_2in13_V4_HEIGHT / 2, EPD_2in13_V4_WIDTH / 2, message, &Font24, WHITE, BLACK);
    Paint_DrawString_EN(true, EPD_2in13_V4_HEIGHT / 2, (EPD_2in13_V4_WIDTH / 2) + Font24.Height, bgChar, &Font20, WHITE, BLACK);
    uiLastScreen = WARNING_GLUCOSE;
}

// Draws a QR Code onto the screen using text as an input.
void UiTextQrCode(int startX, int startY, const char *link){
    Serial.println("qr");
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, 0, link);

    for (int y = 0; y < qrcode.size; y++) {
        for (int x = 0; x < qrcode.size; x++) {
            Serial.print(qrcode_getModule(&qrcode, x, y) ? "." : " ");
            Paint_DrawPoint(startX + (x * 4), startY + (y * 4), qrcode_getModule(&qrcode, x, y) ? BLACK : WHITE, DOT_PIXEL_3X3, DOT_FILL_AROUND);
        }
        Serial.println();
    }
}

// Draws a QR Code onto the screen using wifi credentials as an input.
void UiWifiQrCode(int startX, int startY, String ssid, String password){
    String data = "WIFI:T:WPA;S:" + ssid + ";P:" + password + ";H:;;";
    UiTextQrCode(startX, startY, data.c_str());
}

// Shows the first screen of the configuration process (The connect to network screen).
void UiWiFiConectionPage(String ssid, String password){
    UiWifiQrCode(10, 5, ssid, password);
    Paint_DrawString_EN(false, 130, 5, "Scan QR code or  connect to wifi:", &Font12, WHITE, BLACK);
    Paint_DrawString_EN(false, 130, (Font12.Height * 2) + 15, ssid.c_str(), &Font12, WHITE, BLACK);
    Paint_DrawString_EN(false, 130, (Font12.Height * 3) + 30, password.c_str(), &Font12, WHITE, BLACK);
    uiLastScreen = CONFIG;
}

// Shows the second screen if the configuration process (The web page screen).
void UiWebPageConectionPage(String pageLink){
    UiTextQrCode(10, 5, pageLink.c_str());
    Paint_DrawString_EN(false, 130, 5, "Scan QR code or  use a web browserto go to:", &Font12, WHITE, BLACK);
    Paint_DrawString_EN(false, 130, (Font12.Height * 3) + 15, pageLink.c_str(), &Font12, WHITE, BLACK);
    uiLastScreen = CONFIG;
}

void UiUpdateMode(){
    Paint_DrawString_EN(true, EPD_2in13_V4_HEIGHT / 2, 15, "Update Mode", &Font24, WHITE, BLACK);
    Paint_DrawString_EN(false, 0, 15 + (Font24.Height / 2), "Use platformio to flash the new image to the device.", &Font12, WHITE, BLACK);
}