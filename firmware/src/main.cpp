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

RTC_DATA_ATTR bool noDataPrev = false;
RTC_DATA_ATTR bool wifiTimoutPrev = false;
RTC_DATA_ATTR int wakeupTime = 0;
RTC_DATA_ATTR bool noWifiPrev = false;
RTC_DATA_ATTR int dexErrors = 0;
RTC_DATA_ATTR bool displayUpdateNeeded = false;

RTC_DATA_ATTR int wifiSignalStrength = 0;

// Sleep times.
const int NO_TIME_SLEEP = 5;
const int NO_DATA_SLEEP = 5*60;
const int RETRY_NO_DATA_SLEEP = 10;
const int RETRY_TIMOUT_WIFI_SLEEP = 10;
const int TIMOUT_WIFI_SLEEP = 30;
const int NO_WIFI_SLEEP = 5*60;
const int RETRY_NO_WIFI_SLEEP = 20;
const int DS_ERROR_SLEEP = 20;

void NoData(unsigned long currentTime, Config config){
    if (noDataPrev){
        Serial.println("No Data!");
        DisplayGlucose(prevGl, true, "No Data", displayUpdateNeeded, wifiSignalStrength);
        if (!
            GetBooleanValue("rel-timestamp", config)){
            wakeupTime = currentTime + NO_DATA_SLEEP;
            Sleep(NO_DATA_SLEEP);
        }
        else{
            wakeupTime = currentTime + 60;
            Sleep(60);
        }
    }
    else{
        noDataPrev = true;
        Serial.print("No new reading yet.");
        wakeupTime = currentTime + RETRY_NO_DATA_SLEEP;
        Sleep(RETRY_NO_DATA_SLEEP);
    }
    
}

void PrintGlucose(GlucoseReading gl){
    Serial.print("MMOL/L: ");
    Serial.println(gl.bg);
    Serial.print("MMOL/L Delta: ");
    Serial.println(gl.delta);
    Serial.print("MG/DL: ");
    Serial.println(gl.mgdl);
    Serial.print("MG/DL Delta: ");
    Serial.println(gl.mgdlDelta);
    Serial.print("Glucose Timestamp: ");
    Serial.print(gl.timestamp);
    Serial.print(", Tz adjusted: ");
    Serial.println(gl.tztimestamp);
}

GlucoseReading GetBG(Config config){
    GlucoseReading gl;

    bool sucess = false;
    for (int i = 0; i < 2; i++){
        if (BgDataSourceInit(config)){
            sucess = true;
            break;
        }
    }
    if (!sucess){
        Serial.println("Bg Data source init error");
        DisplayGlucose(prevGl, true, "DS Error", updateNeeded, wifiSignalStrength);
        Sleep(DS_ERROR_SLEEP);
    }
    
    // Try twice at getting the reading until giving up and sleeping for a couple seconds.
    sucess = false;
    for (int i = 0; i < 2; i++){
        if (RetrieveGlDataSource()){
            gl = GlDataSource();
            sucess = true;
            break;
        }
    }
    if (!sucess){
        // DS stands for data source.
        Serial.println("Bg Data source get error");
        DisplayGlucose(prevGl, true, "DS Error", updateNeeded, wifiSignalStrength);
        Sleep(DS_ERROR_SLEEP);
    }
    
    PrintGlucose(gl);

    return gl;
}

bool UpdateBg(){
    bool updateBg;
    int minsSinceLastReading = round((double)((wakeupTime - prevGl.timestamp)/60));
    updateBg = (
        (wakeupTime - prevGl.timestamp > 5*60 -2) && !noDataPrev) || 
        (((double)minsSinceLastReading/5 == floor((double)minsSinceLastReading/5)) && noDataPrev
    );
    return updateBg;
}

void UpdateDisplay(Config config){
    // Get the current epoch time.
    unsigned long currentTime = wakeupTime;

    // Get the blood glucose reading from dexcom.
    GlucoseReading gl = prevGl;
    if (UpdateBg() || prevGl.bg == 0.0 || wakeupTime == 0){
        currentTime = GetEpoch();
        if (currentTime == 0){
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

        gl = GetBG(config);
        gl.minsSinceReading = round((currentTime - gl.timestamp) / 60);
    }
    else{
        gl.minsSinceReading = round((wakeupTime - gl.timestamp) / 60);
    }
    prevGl = gl;

    if (GetBooleanValue("update-check", config)){
        displayUpdateNeeded = CheckForUpdate();
        if (GetBooleanValue("auto-update", config) && displayUpdateNeeded){
            OtaUpdate();
        }
    }

    // no longer need wifi so turn it off to save some power.
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    // SLEEP!
    int timeUntilNewReading = (5 * 60) - (currentTime - gl.timestamp);

    // sleep for the next reading.
    
    if(timeUntilNewReading < 0){NoData(currentTime, config);}
    else {
        DisplayGlucose(gl, false, "", displayUpdateNeeded, wifiSignalStrength);
        if (GetBooleanValue("rel-timestamp", config)){
            // Work out how long to sleep.
            
            int sleepTimeRemainder = timeUntilNewReading - (round(timeUntilNewReading/60)*60);
            if (sleepTimeRemainder){
                wakeupTime = currentTime + sleepTimeRemainder;
                Sleep(sleepTimeRemainder);
            }
            else{
                wakeupTime = currentTime + 60;
                Sleep(60);
            }
        }
        else{
            wakeupTime = currentTime + timeUntilNewReading+4;
            Sleep(timeUntilNewReading+4);
        }
    } //TODO: add 2 to sleep time if its missing readings.
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

    if (UpdateBg() || prevGl.bg == 0.0 || wakeupTime == 0){
        if (ConnectToNetwork(wifiSsid, wifiPassword)){
            noWifiPrev = false;
            wifiTimoutPrev = false;
            wifiSignalStrength = GetSignalStrength();
            UpdateDisplay(config);
        }
        else{
            wifiSignalStrength = 0;
            WifiNetwork savedNetwork = {wifiSsid, wifiPassword};
            bool savedNetworkExists = SavedNetworkExists(savedNetwork);
            if (savedNetworkExists){
                Serial.println("A saved wifi network exists :)");
                if (wifiTimoutPrev){
                    if (prevGl.bg != 0.0){DisplayGlucose(prevGl, true, "WiFi Timout", displayUpdateNeeded, wifiSignalStrength);}
                    Sleep(TIMOUT_WIFI_SLEEP);
                }
                else{
                    if (prevGl.bg != 0.0){DisplayGlucose(prevGl, false, "", displayUpdateNeeded, wifiSignalStrength);}
                }
                Serial.println("Failed to connect to wifi network (timed out) :(");
                wifiTimoutPrev = true;
                Sleep(RETRY_TIMOUT_WIFI_SLEEP);
            }
            else{
                if (noWifiPrev){
                    if (prevGl.bg != 0.0){DisplayGlucose(prevGl, true, "No WiFi", displayUpdateNeeded, wifiSignalStrength);}
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
    else{
        UpdateDisplay(config);
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
    UiInitConfig(config);
    UpdateInitConfig(config);

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