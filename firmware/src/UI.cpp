#include "epaper/DEV_Config.h"
#include "epaper/EPD.h"
#include "epaper/GUI_Paint.h"
#include <stdlib.h>
#include <Arduino.h>
#include <qrcode.h>
#include "UI.h"
#include <ctime>
#include "config/config_manager.h"
#include <string>
#include "images/arrows.h"
#include "images/icons.h"
#include "update_manager.h"
#include "glucose/bg_datasource.h"

using namespace std;

UBYTE *MainImage;
RTC_DATA_ATTR UiScreen uiLastScreen = NONE;
Config UiConfig;

// Initilise display.
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
// Initilise display configuration.
void UpdateUiConfig(Config config){
    UiConfig = config;
}

// Clear buffer.
void UiFullClear(){
    Paint_SelectImage(MainImage);
    Paint_NewImage(MainImage, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);  	
    Paint_Clear(WHITE);
}
// Refresh the display showing the frame buffer.
void UiShow(){
    EPD_2in13_V4_Display(MainImage);
}
// Partial update the display.
void UiShowPartial(){
    EPD_2in13_V4_Display_Partial(MainImage);
}
// Write the buffer to memory (used when partial updating occasionaly)
void UiWriteToMem(){
    EPD_2in13_V4_Display_Mem(MainImage);
}

void UiClearText(bool centered, int Xstart, int Ystart, const char * pString, sFONT* Font){
    int Xpoint;
    int Ypoint;
    int string_length = strlen(pString);
    if (centered){
        Xpoint = Xstart - ((Font->Width * string_length) / 2);
        Ypoint = Ystart - (Font->Height / 2);
    }
    else{
        Xpoint = Xstart;
        Ypoint = Ystart;
    }
    Paint_ClearWindows(Xpoint, Ypoint, Xpoint + (Font->Width * string_length), Ypoint + Font->Height, WHITE);
}
struct GlucoseReadingString{
    String bg;
    String delta;
    String time;
    const unsigned char* arrow;
};

// Makes the gl into a string version ready for drawing to the screen.
static GlucoseReadingString GetGLChar(GlucoseReading gl, Config config){
    GlucoseReadingString glchr;
    if (!GetBooleanValue("rel-timestamp", UiConfig)){
        bool twelveHourTime = GetBooleanValue("12h-time", config);
        // Get UTC time from the epoch
        time_t epochTime = gl.tztimestamp;
        struct tm *utc = gmtime(&epochTime);
        
        // Make time string
        int minutes = utc->tm_min;
        int hours = utc->tm_hour;
        // ajust hours if 12h time
        bool AM = false;
        if (twelveHourTime){
            if (hours == 12){AM = false;}
            else if (hours > 12){hours -= 12;}
            else {AM = true;}
        }
        string minutesString;
        string hoursString;

        // Statring from a blank string add the 0 at the start if the number of minutes or hours is bellow 10 otherwise the time could look like this: 1:4 instead of this: 01:04
        if (minutes < 10){minutesString += "0";}
        minutesString += to_string(minutes);
        if (hours < 10 && !twelveHourTime){hoursString += "0";}
        hoursString += to_string(hours);

        // Create a string with the hour and minute of the timestamp.
        string timestr = hoursString + ":" + minutesString;
        // add the suffix AM or PM to the 12h time.
        if (twelveHourTime){
            if(AM){timestr += " AM";}
            else{timestr += " PM";}
        }
        glchr.time = timestr.c_str();
    }
    else{
        glchr.time = to_string(gl.minsSinceReading).c_str();
        glchr.time += "Mins";
    }

    // Convert bg into char
    if (GetBooleanValue("mmol-l", UiConfig)){
        char bgChar[5];
        sprintf(bgChar, "%.1f", gl.bg);
        glchr.bg = bgChar;
    }
    else{
        glchr.bg = to_string(gl.mgdl).c_str();
    }

    // Add a plus to the delta if it is positive.
    if (GetBooleanValue("mmol-l", UiConfig)){
        char deltaString[5];
        if (gl.delta >= 0.0){ sprintf(deltaString, "+%.1f", gl.delta); }
        else{ sprintf(deltaString, "%.1f", gl.delta); }
        glchr.delta = deltaString;
    }
    else{
        if (gl.mgdlDelta >= 0){glchr.delta = ("+" + to_string(gl.mgdlDelta)).data(); }
        else{glchr.delta = to_string(gl.mgdlDelta).data();}
    }
    
    const unsigned char* arrow;
    String trend = String(gl.trendDescription);
    if (trend == "SingleUp" || trend == "DoubleUp") arrow = ArrowN_bits;
    else if (trend == "FortyFiveUp") arrow = ArrowNE_bits;
    else if (trend == "Flat") arrow = ArrowE_bits;
    else if (trend == "FortyFiveDown") arrow = ArrowSE_bits;
    else if (trend == "SingleDown" || trend == "DoubleDown") arrow = ArrowS_bits;
    else{
        arrow = ArrowE_bits;
        Serial.println("Trent Description not recognised.");
    }
    glchr.arrow = arrow;
    
    return glchr;
}

// Draw Glucose screen (includes glucose level, delta and timestamp)
void UiGlucose(GlucoseReading gl){
    GlucoseReadingString glstr = GetGLChar(gl, UiConfig);
    bool arrowEnabled = GetBooleanValue("trend-arrow", UiConfig);
    int glucoseOffset = 0;
    if (glstr.bg.length() <= 3){
        if (arrowEnabled){glucoseOffset = (EPD_2in13_V4_HEIGHT-((Font80.Width*glstr.bg.length())+60))/2;}
        else{glucoseOffset = (EPD_2in13_V4_HEIGHT-(Font80.Width*glstr.bg.length()))/2;}
    }
    // Draw bg
    Paint_DrawString_EN(false, 0+glucoseOffset, (EPD_2in13_V4_WIDTH / 2) - (Font80.Height/2), glstr.bg.c_str(), &Font80, BLACK, WHITE);
    // Draw Delta
    if (GetBooleanValue("delta", UiConfig)){
        Paint_DrawString_EN(true, 215, 105, glstr.delta.c_str(), &Font20, BLACK, WHITE);
    }
    // Draw timestamp
    if (GetBooleanValue("timestamp", UiConfig)){
        Paint_DrawString_EN(true, ((glstr.bg.length()*Font80.Width)/2)+glucoseOffset, 105, glstr.time.c_str(), &Font20, BLACK, WHITE);
    }
    // Draw arrow
    if (arrowEnabled){
        Paint_DrawImage(glstr.arrow, 33, 192-(Font80.Width * (glstr.bg.length()-4)*-1)+glucoseOffset, 60, 60);
    }
    uiLastScreen = GLUCOSE;
}

// Clear Glucose screen (includes glucose level, delta and timestamp) ready for partial update.
void UiClearGlucose(GlucoseReading gl){
    GlucoseReadingString glstr = GetGLChar(gl, UiConfig);
    bool arrowEnabled = GetBooleanValue("trend-arrow", UiConfig);
    int glucoseOffset = 0;
    if (glstr.bg.length() <= 3){
        if (arrowEnabled){glucoseOffset = (EPD_2in13_V4_HEIGHT-((Font80.Width*glstr.bg.length())+60))/2;}
        else{glucoseOffset = (EPD_2in13_V4_HEIGHT-(Font80.Width*glstr.bg.length()))/2;}
    }
    // Draw bg
    UiClearText(false, 0+glucoseOffset, (EPD_2in13_V4_WIDTH / 2) - (Font80.Height/2), glstr.bg.c_str(), &Font80);
    // Draw Delta
    if (GetBooleanValue("delta", UiConfig)){
        UiClearText(true, 215, 105, glstr.delta.c_str(), &Font20);
    }
    // Draw timestamp
    if (GetBooleanValue("timestamp", UiConfig)){
        UiClearText(true, ((glstr.bg.length()*Font80.Width)/2)+glucoseOffset, 105, glstr.time.c_str(), &Font20);
    }
    // Draw arrow
    if (arrowEnabled){
        int arrowXPos = (192-(Font80.Width * (glstr.bg.length()-4)*-1)) + glucoseOffset;
        int arrowYPos = 33;
        Paint_ClearWindows(arrowXPos, arrowYPos, arrowXPos + 60, arrowYPos - 60, WHITE);
    }
    uiLastScreen = GLUCOSE;
}

// Line through the glucose text.
void UiGlucoseStrikethrough(){
    Paint_DrawLine(0, (EPD_2in13_V4_WIDTH / 2)+5, EPD_2in13_V4_HEIGHT, (EPD_2in13_V4_WIDTH / 2)+5, WHITE, DOT_PIXEL_7X7, LINE_STYLE_SOLID);
    Paint_DrawLine(0, (EPD_2in13_V4_WIDTH / 2)+5, EPD_2in13_V4_HEIGHT, (EPD_2in13_V4_WIDTH / 2)+5, BLACK, DOT_PIXEL_5X5, LINE_STYLE_SOLID);
}
// Clear the line through the glucose text.
void UiGlucoseClearStrikethrough(){
    Paint_ClearWindows(0, (EPD_2in13_V4_WIDTH / 2), EPD_2in13_V4_HEIGHT, (EPD_2in13_V4_WIDTH / 2)+10, WHITE);
}

// Warning message above glucose.
void UiGlucoseWarning(String warning){
    Paint_DrawString_EN(true, EPD_2in13_V4_HEIGHT/2, Font20.Height/2, warning.c_str(), &Font20, BLACK, WHITE);
}
// Clear warning message above glucose.
void UiGlucoseClearWarning(String warning){
    UiClearText(true, EPD_2in13_V4_HEIGHT/2, Font20.Height/2, warning.c_str(), &Font20);
}

void UiUpdateIcon(){
    Paint_DrawImage(UpdateIcon_bits, EPD_2in13_V4_WIDTH - 20, 1, 20, 20);
}
void UiClearUpdateIcon(){
    Paint_ClearWindows(1, 0, 20 + 1, 20, WHITE);
}

void UiWiFiIcon(int value){
    const unsigned char* image;
    switch (value)
    {
    case 0:
        image = NoWifiIcon_bits;
        break;
    case 1:
        image = WifiIcon1_bits;
        break;
    case 2:
        image = WifiIcon2_bits;
        break;
    case 3:
        image = WifiIcon3_bits;
        break;
    
    default:
        image = NoWifiIcon_bits;
        break;
    }
    Paint_DrawImage(image, EPD_2in13_V4_WIDTH - 20, 220, 20, 20);
}

void UiClearWiFiIcon(){
    Paint_ClearWindows(220, 0, 20 + 220, 20, WHITE);
}

// Displays a warning message on screen with subtext.
void UiWarning(const char *message, const char *subtext){
    Paint_DrawString_EN(true, EPD_2in13_V4_HEIGHT / 2, EPD_2in13_V4_WIDTH / 2, message, &Font24, WHITE, BLACK);
    Paint_DrawString_EN(true, EPD_2in13_V4_HEIGHT / 2, (EPD_2in13_V4_WIDTH / 2) + Font24.Height, subtext, &Font8, WHITE, BLACK);
    uiLastScreen = WARNING; 
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
    string versionString = "Version: V" + to_string(VERSION_MAJOR) + "." + to_string(VERSION_MINOR) + "." + to_string(VERSION_REVISION);
    if (VERSION_BUILD != 0){versionString += "-" + to_string(VERSION_BUILD);}
    Paint_DrawString_EN(true, EPD_2in13_V4_HEIGHT / 2, 22+ (Font24.Height / 2), versionString.c_str(), &Font16, WHITE, BLACK);
    Paint_DrawString_EN(false, 0, 15 + (Font24.Height / 2) + (Font16.Height), "Press the button on the side of the device to update from github or use platformio to flash from usb.", &Font12, WHITE, BLACK);
    uiLastScreen = UPDATE;
}

void UiSetupScreen(){
    Paint_DrawString_EN(true, EPD_2in13_V4_HEIGHT / 2, 15, "Welcome!", &Font24, WHITE, BLACK);
    Paint_DrawString_EN(false, 0, Font16.Height + Font24.Height + 5, "Press the configuration button on the left of the device to begin setup.", &Font16, WHITE, BLACK);
    uiLastScreen = SETUP;
}

void UiSetupFinish(){
    Paint_DrawString_EN(true, EPD_2in13_V4_HEIGHT / 2, 10, "Setup Finished!", &Font20, BLACK, WHITE);
    Paint_DrawString_EN(false, 0, Font16.Height + Font20.Height + 5, "Press the button againto restart the device.", &Font16, WHITE, BLACK);
    uiLastScreen = SETUP;
}