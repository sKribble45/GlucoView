#include <WiFi.h>
#include <Arduino.h>
#include "dexcom/Dexcom_follow.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "epaper/DEV_Config.h"
#include "epaper/EPD.h"
#include "epaper/GUI_Paint.h"
#include <stdlib.h>
#include "UI.h"
#include "config/config_manager.h"
#include "wifi_manager/wifi_manager.h"
#include <list>
#include <unordered_map>
#include <Preferences.h>
#include "dexcom/dexcom_account.h"
#include "driver/rtc_io.h"
#include "time.h"
using namespace std;


const bool TEST_CONFIG = false;
esp_sleep_wakeup_cause_t wakeup_reason;
RTC_DATA_ATTR GlucoseReading prevGl;
RTC_DATA_ATTR bool noDataPrev = false;
RTC_DATA_ATTR bool wifiTimoutPrev = false;
RTC_DATA_ATTR int wakeupTime = 0;
RTC_DATA_ATTR bool noWifiPrev = false;
RTC_DATA_ATTR int partialUpdates = 0;

RTC_DATA_ATTR int dexErrors = 0;


// sleep times
const int NO_TIME_SLEEP = 20;
const int NO_DATA_SLEEP = 5*60;
const int SHORT_NO_DATA_SLEEP = 20;
const int RETRY_TIMOUT_WIFI_SLEEP = 10;
const int TIMOUT_WIFI_SLEEP = 30;
const int NO_WIFI_SLEEP = 5*60;
const int RETRY_NO_WIFI_SLEEP = 20;
const int DEXCOM_ERROR_SLEEP = 1*60;

UBYTE *Epaper_Image;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

#define BUTTON_PIN D5

void Sleep(int sleep_seconds){
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.print("Going to sleep for ");
    Serial.print(sleep_seconds);
    Serial.println("s *yawn*");
    EPD_2in13_V4_Sleep(); // tell the screen to go to sleep "good night" *yawn*
    esp_sleep_enable_timer_wakeup(sleep_seconds * 1000000); // tel the esp how long to sleep *big yawn*
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 1);
    rtc_gpio_pullup_dis((gpio_num_t)BUTTON_PIN);
    rtc_gpio_pulldown_en((gpio_num_t)BUTTON_PIN);
    //esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF); // very sleep deep *strech*
    esp_deep_sleep_start(); // COMENSE THE SLEEP!
}
string randomString(size_t length) {
    const string characterSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/*+-=";
    string result;
    srand(time(0));

    for (size_t i = 0; i < length; ++i) {
        result += characterSet[rand() % characterSet.size()];
    }
    return result;
}

void Config(){
    UserConfig config;
    string ApPassword = "glucoview_"+randomString(5);
    HostConfigAP(config, "GlucoView", ApPassword.c_str());
    SaveConfig(config);
}

void NoData(){
    if (noDataPrev){
        Serial.println("No Data!");
        UiFullClear();
        UiWarningGlucose("No Data", prevGl.bg,"No glucose reading from dexcom.");
        UiShow();
        Sleep(NO_DATA_SLEEP);
    }
    else{
        noDataPrev = true;
        Serial.print("No new reading yet.");
        Sleep(SHORT_NO_DATA_SLEEP);
    }
    
}


unsigned long GetEpoch(){
    unsigned long currentTime;

    const int forceUpdateLimit = 10;
    int forceUpdates = 0;
    timeClient.begin();
    while(!timeClient.update() && forceUpdates <= forceUpdateLimit) {
        timeClient.forceUpdate();
        Serial.println("Timeclient not updated, forcing update.");
        forceUpdates ++;
        delay(1000);
    }
    if (forceUpdates > forceUpdateLimit){
        if (wakeupTime > 0){
            Serial.println("Timeclient not updating, using old timestamp + the time it slept (less accurate)");
            // calculate the previous wakeup time + the time it slept 
            // This is less accurate because it could be doing things in between that take a few seconds
            currentTime = wakeupTime;
        }
        else{
            Serial.println("No previous time data.");
            Sleep(NO_TIME_SLEEP);
        }
    }
    else{
        currentTime = timeClient.getEpochTime(); ////+ 1 * 60 *60;
    }


    return currentTime;
}

void PrintGlucose(GlucoseReading gl){
    Serial.print("BG: ");
    Serial.println(gl.bg);
    Serial.print("Delta: ");
    Serial.println(gl.delta);
    Serial.print("Glucose Timestamp: ");
    Serial.print(gl.timestamp);
    Serial.print(", Tz adjusted: ");
    Serial.println(gl.tztimestamp);
    Serial.print("Trend Arrow: ");
    Serial.println(gl.trend_Symbol);
}

GlucoseReading GetBG(UserConfig config){
    Follower follower(true, config.dexcomUsername, config.dexcomPassword);
    // follower.getNewSessionID();
    if (!follower.getNewSessionID()){
        if (dexErrors == 0){
            UiFullClear();
            UiWarning("Dexcom Error", "Error connecting to dexcom servers");
            UiShow();
        }
        if (dexErrors < 3){dexErrors++;}
        else{
            UiFullClear(); 
            UiWarning("Dexcom Error", "You may want to consider re-pairing using the pairing button on the left side of the device as this error can be caused by inputing your dexcom credentials wrong."); 
            UiShow();
        }
        Sleep(DEXCOM_ERROR_SLEEP);
    }
    else{dexErrors = 0;}
    follower.GlucoseLevelsNow();

    GlucoseReading gl = follower.GlucoseNow;
    PrintGlucose(gl);

    // if the reading hasnt changed yet call the no data function.
    if (gl.timestamp == prevGl.timestamp){NoData();}
    else{noDataPrev = false;}
    // work out delta or if it is the first reading set delta to 0.0
    if (prevGl.bg != 0.0){ gl.delta = gl.bg - prevGl.bg; }
    else{ gl.delta = 0.0; }


    return gl;
}

void UpdateDisplay(UserConfig config){
    // Get the blood glucose level from dexcom.
    GlucoseReading gl = GetBG(config);

    unsigned long currentTime = GetEpoch();

    // no longer need wifi so turn it off to save some power.
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    // display the glucose and delta on the display.
    
    if (uiLastScreen != GLUCOSE || partialUpdates >= 10){
        UiGlucose(gl);
        UiShow();
        partialUpdates = 0;
    }
    else{
        UiGlucose(prevGl);
        UiWriteToMem();

        UiClearGlucose(prevGl);
        UiGlucose(gl);
        UiShowPartial();
        partialUpdates ++;
    }
    // set the previous glucose to current glucose.
    prevGl = gl;
    //TODO: Make this neater.
    

    // SLEEP!
    // work out the time it has to sleep for the next reading.
    int sleepTime = (5 * 60) - (currentTime - gl.timestamp);

    // sleep for the next reading.
    wakeupTime = currentTime + sleepTime;
    if (sleepTime < 0 && sleepTime > - 120){Sleep(10);}
    else if(sleepTime < 0){NoData();}
    else {Sleep(sleepTime +2);} //TODO: add 2 to sleep time if its missing readings.
}

void OnStart(UserConfig config) {
    if (ConnectToNetwork(config.wifiSsid, config.wifiPassword)){
        noWifiPrev = false;
        wifiTimoutPrev = false;
        UpdateDisplay(config);
    }
    else{
        //TODO: In the userconfig struct make the wifi credentials into a WifiNetwork struct
        WifiNetwork savedNetwork = {config.wifiSsid, config.wifiPassword};
        bool savedNetworkExists = SavedNetworkExists(savedNetwork);
        // if (!savedNetworkExists && !noWifiPrev){
        //     // try again :D
        //     savedNetworkExists = SavedNetworkExists(savedWifiNetwork);
        // }
        if (savedNetworkExists){
            Serial.println("A saved wifi network exists :)");
            if (wifiTimoutPrev){
                UiFullClear();
                UiWarning("No Data", "Timed out when connecting to wifi network.");
                UiShow();
                Sleep(TIMOUT_WIFI_SLEEP);
            }
            Serial.println("Failed to connect to wifi network (timed out) :(");
            wifiTimoutPrev = true;
            Sleep(RETRY_TIMOUT_WIFI_SLEEP);
        }
        else{
            if (noWifiPrev){
                UiFullClear();
                UiWarning("No Data", "No network found with matching ssid");
                UiShow();
                Sleep(NO_WIFI_SLEEP);
            }
            else{
                Serial.println("No saved wifi network exists :(");
                Sleep(RETRY_NO_WIFI_SLEEP);
            }
            noWifiPrev = true;
        }
    }
}

void UpdateMode(){
    Serial.print("Started Update mode, Waiting for update");
    UiUpdateMode();
    UiShow();
    while (true) {delay(1000);};
}

void setup(){
    Serial.begin(115200);
    wakeup_reason = esp_sleep_get_wakeup_cause();
    pinMode(BUTTON_PIN, INPUT_PULLDOWN);
    int buttonValue = digitalRead(BUTTON_PIN);

    Serial.println("Initialise display");
    DispInit();
    Serial.println("Display initilised.");

    // If it hasnt woken up from sleep because of the button press (so not pairing) and the button is down (when it is plugged )
    if (wakeup_reason != ESP_SLEEP_WAKEUP_EXT0 && buttonValue) {UpdateMode();}
    UserConfig config;
    LoadConfig(config);
    if (!ConfigExists(config) || TEST_CONFIG || wakeup_reason == ESP_SLEEP_WAKEUP_EXT0){
        Serial.println("Entering Configuration");
        Config();
        ESP.restart();
    }

    OnStart(config);
}


void loop(){
    // :)
}