#include <WiFi.h>
#include <Arduino.h>
#include "dexcom/Dexcom_follow.h"
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
#include <nvs_flash.h>
#include "images/arrows.h"
#include "glucose_screen.h"
#include "github/github_api.h"
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

UBYTE *Epaper_Image;

#define BUTTON_PIN D5

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

GlucoseReading GetBG(Config config){
    Follower follower(true, get<String>(config["dex-username"]), get<String>(config["dex-password"]));

    if (!follower.getNewSessionID()){
        
        // On failure to connect to the dexcom servers display no data.
        if (dexErrors == 1){
            DisplayGlucose(prevGl, true, "Dexcom Error", displayUpdateNeeded, wifiSignalStrength);
        }
        if (dexErrors < 3){dexErrors++;}
        // if there has been 3 consecutive dexcom errors tell the user that they may want to update their credentials.
        else{
            UiFullClear(); 
            UiWarning("Dexcom Error", "You may want to consider re-pairing using the pairing button on the left side of the device as this error can be caused by inputing your dexcom credentials incorrectly."); 
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

    // if the reading hasnt changed yet call the no data function.
    if (gl.timestamp == prevGl.timestamp){NoData();}
    else{noDataPrev = false;}
    // work out delta or if it is the first reading set delta to 0.0
    if (prevGl.bg != 0.0){ gl.delta = gl.bg - prevGl.bg; }
    else{ gl.delta = 0.0; }


    return gl;
}



void UpdateDisplay(Config config){
    // Get the blood glucose reading from dexcom.
    GlucoseReading gl = GetBG(config);

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

    displayUpdateNeeded = CheckForUpdates();
    

    // no longer need wifi so turn it off to save some power.
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    DisplayGlucose(gl, false, "", displayUpdateNeeded, wifiSignalStrength);
    

    // SLEEP!
    // work out the time it has to sleep for the next reading.
    int sleepTime = (5 * 60) - (currentTime - gl.timestamp);

    // sleep for the next reading.
    wakeupTime = currentTime + sleepTime;
    if (sleepTime < 0 && sleepTime > -120){Sleep(10);}
    else if(sleepTime < 0){NoData();}
    else {Sleep(sleepTime+2);} //TODO: add 2 to sleep time if its missing readings.
}

void OnStart(Config config) {
    String wifiSsid = getStringValue("wifi-ssid", config);
    String wifiPassword = getStringValue("wifi-ssid", config);
    Serial.print("ssid: ");
    Serial.print(wifiSsid);
    Serial.print(" , password: ");
    Serial.println(wifiPassword);

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
                DisplayGlucose(prevGl, true, "WiFi Timout", displayUpdateNeeded, wifiSignalStrength);
                Sleep(TIMOUT_WIFI_SLEEP);
            }
            else{
                DisplayGlucose(prevGl, false, "", displayUpdateNeeded, wifiSignalStrength);
            }
            Serial.println("Failed to connect to wifi network (timed out) :(");
            wifiTimoutPrev = true;
            Sleep(RETRY_TIMOUT_WIFI_SLEEP);
        }
        else{
            if (noWifiPrev){
                DisplayGlucose(prevGl, true, "No WiFi", displayUpdateNeeded, wifiSignalStrength);
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
    while (digitalRead(BUTTON_PIN)){delay(50);}
    while (true) {
        if (digitalRead(BUTTON_PIN)){
            UiFullClear();
            UiWarning("Are you sure", "erasing the flash is perminent and will remove any configuration that is stored on the device. Press button to continue");
            UiShow();
            while (digitalRead(BUTTON_PIN)){delay(50);}
            while (!digitalRead(BUTTON_PIN)){delay(50);}
            nvs_flash_erase(); // erase the NVS partition.
            nvs_flash_init(); // init the NVS partition.
            ESP.restart();
        }
    };
}

void StartConfiguration(Config &config){
    string ApPassword = "glucoview_"+RandomString(5);
    String ApSsid = "GlucoView" + GetSerialNumber();
    HostConfigAP(config, ApSsid.c_str(), ApPassword.c_str());
    SaveConfig(config);
}

void ConfigurationFinish(){
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
    delay(2000);

    Config config;
    LoadConfig(config);
    PrintConfigValues(config);
    // Load config to the ui.
    UiInitConfig(config);

    // Setup screen when you start the device for the first time.
    if (!SerialNumberExists() || !ConfigExists(config)){
        UiSetupScreen();
        UiShow();
        while (!digitalRead(BUTTON_PIN)){delay(50);}
        RandomiseSerialNumber();
        delay(50);
        StartConfiguration(config);
        ConfigurationFinish();
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