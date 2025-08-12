#include <WiFi.h>
#include <Arduino.h>
#include "dexcom/Dexcom_follow.h"
#include <WiFiUdp.h>
#include <stdlib.h>
#include "UI.h"
#include "config/config_manager.h"
#include "wifi_manager/wifi_manager.h"
#include "driver/rtc_io.h"
#include <nvs_flash.h>
#include "images/arrows.h"
#include "glucose_screen.h"
#include "update/update_manager.h"
#include "epoch_time.h"
#include "sleep.h"
using namespace std;

esp_sleep_wakeup_cause_t wakeup_reason;
RTC_DATA_ATTR GlucoseReading prevGl;

RTC_DATA_ATTR bool noDataPrev = false;
RTC_DATA_ATTR bool wifiTimoutPrev = false;
RTC_DATA_ATTR int wakeupTime = 0;
RTC_DATA_ATTR bool noWifiPrev = false;
RTC_DATA_ATTR int dexErrors = 0;
RTC_DATA_ATTR bool displayUpdateNeeded = false;

int wifiSignalStrength = 0;

// Sleep times.
const int NO_TIME_SLEEP = 20;
const int NO_DATA_SLEEP = 5*60;
const int SHORT_NO_DATA_SLEEP = 20;
const int RETRY_TIMOUT_WIFI_SLEEP = 10;
const int TIMOUT_WIFI_SLEEP = 30;
const int NO_WIFI_SLEEP = 5*60;
const int RETRY_NO_WIFI_SLEEP = 20;
const int DEXCOM_ERROR_SLEEP = 1*60;

void NoData(){
    if (noDataPrev){
        Serial.println("No Data!");
        DisplayGlucose(prevGl, true, "No Data", displayUpdateNeeded, wifiSignalStrength);
        Sleep(NO_DATA_SLEEP);
    }
    else{
        noDataPrev = true;
        Serial.print("No new reading yet.");
        Sleep(SHORT_NO_DATA_SLEEP);
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
    Serial.print("Trend Arrow: ");
    Serial.println(gl.trend_Symbol);
}

GlucoseReading GetBG(Config config){
    Follower follower(getBooleanValue("ous", config), getStringValue("dex-username", config), getStringValue("dex-password", config));

    if (!follower.getNewSessionID()){
        // On failure to connect to the dexcom servers display no data.
        if (dexErrors == 1){
            DisplayGlucose(prevGl, true, "Dex Error", displayUpdateNeeded, wifiSignalStrength);
        }
        if (dexErrors < 3){dexErrors++;}
        // if there has been 3 consecutive dexcom errors tell the user that they may want to update their credentials.
        else{
            UiFullClear(); 
            UiWarning("Dex Error", "You may want to consider re-pairing using the pairing button on the left side of the device as this error can be caused by inputing your dexcom credentials incorrectly."); 
            UiShow();
        }
        if (dexErrors == 0){
            if (!follower.getNewSessionID()){Sleep(DEXCOM_ERROR_SLEEP);}
        }
        else{Sleep(DEXCOM_ERROR_SLEEP);}
    }
    else{dexErrors = 0;}
    follower.GlucoseLevelsNow();

    GlucoseReading gl = follower.GlucoseNow;
    PrintGlucose(gl);

    return gl;
}

void UpdateDisplay(Config config){
    // Get the current epoch time.
    unsigned long currentTime = GetEpoch();
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

    // Get the blood glucose reading from dexcom.
    GlucoseReading gl = prevGl;
    if ((currentTime - prevGl.timestamp <= 0) || prevGl.bg == 0.0){
        gl = GetBG(config);
        gl.minsSinceReading = round((currentTime - gl.timestamp) / 60);
        prevGl = gl;
    }

    if (getBooleanValue("update-check", config)){
        displayUpdateNeeded = CheckForUpdate();
        if (getBooleanValue("auto-update", config) && displayUpdateNeeded){
            OtaUpdate();
        }
    }

    // no longer need wifi so turn it off to save some power.
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    
    // SLEEP!
    int timeUntilNewReading = (5 * 60) - (currentTime - gl.timestamp);

    // sleep for the next reading.
    wakeupTime = currentTime + timeUntilNewReading;
    if (timeUntilNewReading < 0 && timeUntilNewReading > -120){Sleep(10);}
    else if(timeUntilNewReading < 0){NoData();}
    else {
        DisplayGlucose(gl, false, "", displayUpdateNeeded, wifiSignalStrength);
        if (getBooleanValue("rel-timestamp", config)){
            // Work out how long to sleep.
            int sleepTimeRemainder = timeUntilNewReading - (round(timeUntilNewReading/60)*60);
            if (sleepTimeRemainder){Sleep(sleepTimeRemainder);}
            else{Sleep(60);}
        }
        else{
            Sleep(timeUntilNewReading);
        }
    } //TODO: add 2 to sleep time if its missing readings.
}

void OnStart(Config config) {
    String wifiSsid = getStringValue("wifi-ssid", config);
    String wifiPassword = getStringValue("wifi-password", config);
    #if DEBUG
        Serial.print("ssid: ");
        Serial.print(wifiSsid);
        Serial.print(" , password: ");
        Serial.println(wifiPassword);
    #endif

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