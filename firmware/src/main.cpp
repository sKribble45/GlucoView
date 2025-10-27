#include <WiFi.h>
#include <Arduino.h>
#include <WiFiUdp.h>
#include <stdlib.h>
#include "UI.h"
#include "config/config_manager.h"
#include "wifi_manager.h"
#include "driver/rtc_io.h"
#include <nvs_flash.h>
#include "images/arrows.h"
#include "glucose_screen.h"
#include "update_manager.h"
#include "epoch_time.h"
#include "sleep.h"
#include "glucose/bg_datasource.h"
#include "config/config_ap.h"
using namespace std;

esp_sleep_wakeup_cause_t wakeup_reason;
RTC_DATA_ATTR GlucoseReading prevGl;

time_t rtcTime;

// Sleep times.
const int NO_TIME_SLEEP = 5;
const int NO_DATA_SLEEP = 5*60;
const int RETRY_NO_DATA_SLEEP = 15;
const int RETRY_TIMOUT_WIFI_SLEEP = 10;
const int WIFI_TIMEOUT_SLEEP = 30;
const int NO_WIFI_SLEEP = 5*60;
const int RETRY_NO_WIFI_SLEEP = 20;
const int DS_ERROR_SLEEP = 20;

const int SLEEP_TIME_DELAY = 4;

enum State{
    STATE_NORMAL,
    STATE_NO_DATA,
    STATE_NO_DATA_RETRY,
    STATE_NO_WIFI,
    STATE_WIFI_TIMEOUT,
    STATE_INCORRECT_WIFI,
    STATE_DS_ERROR
};

RTC_DATA_ATTR enum State state = STATE_NORMAL;

void NoData(unsigned long currentTime, Config config){
    if (state == STATE_NO_DATA_RETRY || state == STATE_NO_DATA){
        state = STATE_NO_DATA;
        Serial.println("No Data");
        DisplayError("No Data", true);
        if (!GetBooleanValue("rel-timestamp", config)){
            Sleep(NO_DATA_SLEEP);
        }
        else{
            Sleep(60);
        }
    }
    else{
        state = STATE_NO_DATA_RETRY;
        Serial.println("No new reading yet.");
        Sleep(RETRY_NO_DATA_SLEEP);
    }
}

bool ShouldUpdateBg(unsigned long rtcTime){
    bool updateBg;
    double minsSinceLastReading = round((rtcTime - prevGl.timestamp)/60);
    updateBg = (
        // If it is 5m after the last reading.
        (state == STATE_NORMAL && (rtcTime - prevGl.timestamp > 5*60 - SLEEP_TIME_DELAY)) || 
        // Check if its a multiple of 5 if the state is No Data.
        (state == STATE_NO_DATA && minsSinceLastReading/5 == floor(minsSinceLastReading/5)) ||
        (state == STATE_NO_DATA_RETRY)
    );
    return updateBg;
}

void RelTimestampSleep(int timeUntilNewReading){
    // Work out how long to sleep.
    int sleepTimeRemainder = timeUntilNewReading - (round(timeUntilNewReading/60)*60);
    if (sleepTimeRemainder == timeUntilNewReading){
        Sleep(sleepTimeRemainder + SLEEP_TIME_DELAY);
    }
    if (sleepTimeRemainder > 0){
        Sleep(sleepTimeRemainder);
    }
    else{
        Sleep(60);
    }
}

void UpdateDisplayGlucose(Config config){
    // Get the current epoch time.
    unsigned long currentTime = rtcTime;

    // Get the blood glucose reading from dexcom.
    GlucoseReading gl = prevGl;
    currentTime = GetEpoch();
    Serial.print("Epoch: ");
    Serial.println(currentTime);
    // Check if ntp time worked
    if (currentTime == 0){
        // If the ntp time dosnt work use inbuilt rtc if it has been set
        if (rtcTime > 10){
            Serial.println("Timeclient not updating, using built in rtc time.");
            currentTime = rtcTime;
        }
        else{
            // Reset as the rtc has not been set to the time via wifi.
            Serial.println("No previous time data.");
            Sleep(NO_TIME_SLEEP);
        }
    }
    if (GetGl()){
        gl = GlNow();
    }
    else{
        Serial.println("DS Error");
        DisplayError("DS Error", true);
        Sleep(DS_ERROR_SLEEP);
    }
    
    PrintGlucose(gl);
    prevGl = gl;
    
    // Update checking
    bool updateNeeded = CheckForUpdate();
    if (GetBooleanValue("update-check", config)){
        UpdateGsNeedUpdate(updateNeeded);
    }
    if (GetBooleanValue("auto-update", config) && updateNeeded){
        OtaUpdate();
    }

    // No longer need wifi so turn it off to save some power.
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    // SLEEP!
    
    int timeUntilNewReading = (5 * 60) - (currentTime - gl.timestamp);

    // sleep for the next reading.
    if(timeUntilNewReading < 0){
        gl.minsSinceReading = round((currentTime - gl.timestamp) / 60);
        PrintGlucose(gl);
        DisplayGlucose(gl);

        NoData(currentTime, config);
    }
    else {
        PrintGlucose(gl);
        DisplayGlucose(gl);
        
        state = STATE_NORMAL;
        if (GetBooleanValue("rel-timestamp", config)){
            RelTimestampSleep(timeUntilNewReading);
        }
        else{
            Sleep(timeUntilNewReading + SLEEP_TIME_DELAY);
        }
    }
}

void UpdateDisplay(){
    prevGl.minsSinceReading = round((rtcTime - prevGl.timestamp) / 60);
    
    PrintGlucose(prevGl);
    if (state == STATE_NO_DATA){
        DisplayError("No Data", true, prevGl);
    }
    else{
        DisplayGlucose(prevGl);
    }
    
    int timeUntilNewReading = (5 * 60) - (rtcTime - prevGl.timestamp);

    RelTimestampSleep(timeUntilNewReading);
}

void OnStart(Config config) {
    String wifiSsid = GetStringValue("wifi-ssid", config);
    String wifiPassword = GetStringValue("wifi-password", config);
    #if DEBUG
        Serial.print("ssid: ");
        Serial.print(wifiSsid);
        Serial.print(" , password: ");
        Serial.println(wifiPassword);
    #endif
    
    time(&rtcTime);
    Serial.print("RTC time: ");
    Serial.println(rtcTime);

    if (ShouldUpdateBg(rtcTime) || prevGl.bg == 0.0 || rtcTime <= 10){
        int networkConnectionCode = ConnectToNetwork(wifiSsid, wifiPassword);
        if (networkConnectionCode == 0){
            UpdateGsSignalStrength(GetWifiSignalStrength());
            UpdateDisplayGlucose(config);
        }
        else{
            UpdateGsSignalStrength(0);
            bool savedNetworkExists = SavedNetworkExists(wifiSsid);
            if (savedNetworkExists){
                if (networkConnectionCode == 6){
                    Serial.println("Failed to connect to wifi network (Incorrect Password) :(");
                    if (state != STATE_INCORRECT_WIFI){
                        state = STATE_INCORRECT_WIFI;
                        Sleep(WIFI_TIMEOUT_SLEEP);
                    }
                    else{
                        DisplayError("WiFi Fail", true);
                        Sleep(NO_WIFI_SLEEP);
                    }
                }
                else{
                    Serial.println("Failed to connect to wifi network (Timed out) :(");
                    if (state != STATE_WIFI_TIMEOUT){
                        state = STATE_WIFI_TIMEOUT;
                    }
                    Sleep(WIFI_TIMEOUT_SLEEP);
                    
                    //TODO: Handle Error
                }
            }
            else{
                Serial.println("Failed to connect to wifi network (Not found) :(");
                state = STATE_NO_WIFI;
                Sleep(NO_WIFI_SLEEP);
                //TODO: Handle Error
            }
        }
    }
    else{
        Serial.println("Updating Mins since reading.");
        UpdateDisplay();
    }
}

void StartConfiguration(Config &config){
    String ApPassword = "glucoview_"+GetSerialNumber();
    String ApSsid = "GlucoView" + GetSerialNumber();
    HostConfigAP(config, ApSsid.c_str(), ApPassword.c_str());
    SaveConfig(config);

    UiFullClear();
    UiSetupFinish();
    UiShow();
    while (!digitalRead(BUTTON_PIN)){delay(50);}
    while (digitalRead(BUTTON_PIN)){delay(50);}
    ESP.restart();
}

void setup(){
    Serial.begin(115200);
    // Get the reason it was woken from sleep.
    wakeup_reason = esp_sleep_get_wakeup_cause();
    // Pull down the button pin to ground so it can see a change when shorted to 3.3v
    pinMode(BUTTON_PIN, INPUT_PULLDOWN);
    int buttonValue = digitalRead(BUTTON_PIN);

    Serial.println("Initialise display");
    DispInit();
    Serial.println("Display initilised.");

    // If it hasnt woken up from sleep because of the button press (so not pairing) and the button is down (when it is plugged )
    if (wakeup_reason != ESP_SLEEP_WAKEUP_EXT0 && buttonValue) {UpdateMode();}

    //!! DO NOT REMOVE CODE BEFORE THIS MESSAGE UNLESS YOU KNOW WHAT YOU ARE DOING (you may need to take appart the device to bootstrap so that you can re-program if you do)

    // Delay to connect to serial port.
    #if DEBUG
        delay(2000);
    #endif

    Config config;
    LoadConfig(config);
    #if DEBUG
        PrintConfigValues(config);
    #endif
    // Load config to other components.
    UpdateUiConfig(config);
    UpdateUpdateManagerConfig(config);
    UpdateDataSourceConfig(config);

    // Setup screen when you start the device for the first time.
    if (!SerialNumberExists() || !ConfigExists(config)){
        UiSetupScreen();
        UiShow();
        while (!digitalRead(BUTTON_PIN)){delay(50);}
        RandomiseSerialNumber();
        delay(50);
        StartConfiguration(config);
    }

    // If it was woken up by the button press (or config dosnt exist) enter configuration mode.
    if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0){
        Serial.println("Entering Configuration");
        StartConfiguration(config);
        // Restart the device after configuration.
        ESP.restart();
    }

    // Run the main start code
    OnStart(config);
}


void loop(){
    // :)
}