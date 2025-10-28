#include "ArduinoJson.h"
#include "update_manager.h"
#include <vector>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <iostream>
#include <sstream>
#include <string>
#include <Update.h>
#include "config/config_manager.h"
#include "UI.h"

using namespace std;

const char* repo = "sKribble45/GlucoView";

RTC_DATA_ATTR bool updateNeeded = false;
RTC_DATA_ATTR time_t lastCheckedForUpdates = 0;

Config updateConfig;

// Initilise update configuration.
void UpdateUpdateManagerConfig(Config config){
    updateConfig = config;
}

bool GetReleases(JsonDocument &json, String repo){
    HTTPClient httpClient;

    httpClient.begin("https://api.github.com/repos/"+repo+"/releases");

    int httpCode = httpClient.GET();

    if (httpCode > 0 && httpCode == HTTP_CODE_OK){
        String response = httpClient.getString();
        DeserializationError error = deserializeJson(json, response);
        if (error){
            Serial.print("Error parsing JSON response: ");
            Serial.println(error.c_str());
            httpClient.end();
            return false;
        }
        return true;
    }
    else{return false;}
}

Version ParseStringVersion(String versionString){
    String versionName;
    for (char chr : versionString){
        if (chr != 'V' && chr != 'v'){
            versionName += chr;
        }
    }

    Version version;

    stringstream ss(versionName.c_str());
    string segment;
    // Extract major
    if (getline(ss, segment, '.')) {
        version.major = stoi(segment);
    }
    // Extract minor
    if (getline(ss, segment, '.')) {
        version.minor = stoi(segment);
    }
    // Extract revision
    if (getline(ss, segment, '.')) {
        size_t dashPos = segment.find('-');
        if (dashPos != string::npos) {
            // Split revision and build
            version.revision = stoi(segment.substr(0, dashPos));
            version.build = stoi(segment.substr(dashPos + 1));
        } else {
            version.revision = stoi(segment);
            version.build = 0;
        }
    }

    return version;
}

JsonObject GetLatestRelease(JsonDocument &json){
    JsonObject latestRelease = json[0];
    for (JsonObject release: json.as<JsonArray>()){
        if (!release["prerelease"].as<bool>()){
            latestRelease = release;
            break;
        }
    }
    Version latestVesion = ParseStringVersion(latestRelease["tag_name"]);
    if (GetBooleanValue("beta", updateConfig)){
        for (JsonObject release : json.as<JsonArray>()){
            Version releaseItteratorVersion = ParseStringVersion(release["tag_name"]);
            if (release["prerelease"].as<bool>() 
            && latestVesion.major <= releaseItteratorVersion.major 
            && latestVesion.minor <= releaseItteratorVersion.minor 
            && latestVesion.revision <= releaseItteratorVersion.revision){
                Serial.println("Beta release found");
                latestRelease = release;
                break;
            }
        }
    }
    return latestRelease;
}

// Get version from github (defults to 0.0.0-0)
Version GetVersion(){
    JsonDocument json;
    if (GetReleases(json, repo)){
        JsonObject latestRelease = GetLatestRelease(json);
        String versionNameUnfiltered = latestRelease["tag_name"].as<const char*>();
        return ParseStringVersion(versionNameUnfiltered);
    }
    else{
        return {0,0,0,0};
    }
    
}

bool CheckForUpdate(){
    time_t t = time(0);
    if (t - lastCheckedForUpdates > 30*60 || lastCheckedForUpdates == 0){
        Version latestVersion = GetVersion();
        if (latestVersion.major != 0){
            updateNeeded = (
                latestVersion.major > VERSION_MAJOR || 
                
                (latestVersion.minor > VERSION_MINOR && 
                    latestVersion.major == VERSION_MAJOR) || 

                (latestVersion.revision > VERSION_REVISION && 
                    latestVersion.minor == VERSION_MINOR && 
                    latestVersion.major == VERSION_MAJOR));
            
            if (GetBooleanValue("beta", updateConfig)){
                Serial.println(latestVersion.build);
                updateNeeded == updateNeeded || 
                (latestVersion.build > VERSION_BUILD && 
                    latestVersion.revision == VERSION_REVISION && 
                    latestVersion.minor == VERSION_MINOR && 
                    latestVersion.major == VERSION_MAJOR);
            }
            else if (VERSION_BUILD != 0){
                updateNeeded = true;
            }
            
            lastCheckedForUpdates = t;
        }
    }
    return updateNeeded;
}

String GetFirmwareUrl(){
    String URL = "";
    JsonDocument json;
    if (GetReleases(json, repo)){
        JsonObject latestRelease = GetLatestRelease(json);
        for (JsonObject asset : latestRelease["assets"].as<JsonArray>()){
            if (asset["name"] == FIRMWARE_FILE_NAME){
                URL = String(asset["browser_download_url"]);
            }
        }
    }
    return URL;
}

bool UpdateFromUrl(String url){
    WiFiClientSecure client;
    client.setInsecure(); //TODO: Make more secure.
    HTTPClient http;
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    Serial.println("Starting OTA update...");

    http.begin(client, url);
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        Serial.println(contentLength);
        bool canBegin = Update.begin(contentLength);

        if (canBegin) {
            WiFiClient* stream = http.getStreamPtr();
            size_t written = Update.writeStream(*stream);

            if (written == contentLength) {
                Serial.println("Written : " + String(written) + " successfully");
            } else {
                Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
            }

            if (Update.end()) {
                if (Update.isFinished()) {
                    Serial.println("Update successfully completed.");
                    return true;
                } else {
                    Serial.println("Update not finished? Something went wrong.");
                }
            } else {
                Serial.println("Error Occurred. Error #: " + String(Update.getError()));
            }
        } else {
            Serial.println("Not enough space to begin OTA");
        }
    } else {
        Serial.println("Cannot download firmware. HTTP error code: " + String(httpCode));
    }

    http.end();
    return false;
}

void OtaUpdate(){
    String URL = GetFirmwareUrl();
    if (URL != ""){
        UiFullClear();
        UiWarning("Updating...", "Downloading update... Do not unplug the device.");
        UiShow();
        if (UpdateFromUrl(URL)){
            UiFullClear();
            UiWarning("Update Sucess", "Restarting...");
            UiShow();
            ESP.restart();
        }
        else{
            UiFullClear();
            UiWarning("Update Failed", "HTTP Request failed.");
            UiShow();
        }
        
    }
}

void WaitForButtonPress(){
    if (digitalRead(BUTTON_PIN)){
        while (digitalRead(BUTTON_PIN)){delay(20);}
    }
    while (!digitalRead(BUTTON_PIN)){delay(20);}
    while (digitalRead(BUTTON_PIN)){delay(20);}
}

void UpdateMode(){
    Serial.print("Started Update mode, Waiting for update");
    UiFullClear();
    UiUpdateMode();
    UiShow();

    WaitForButtonPress();

    UiFullClear();
    UiWarning("Updating...", "Connecting to WiFi...");
    UiShow();

    Config config;
    LoadConfig(config);
    UpdateUpdateManagerConfig(config);
    UpdateUiConfig(config);

    String wifiSsid = GetStringValue("wifi-ssid", config);
    String wifiPassword = GetStringValue("wifi-password", config);
    #if DEBUG
        Serial.print("ssid: ");
        Serial.print(wifiSsid);
        Serial.print(" , password: ");
        Serial.println(wifiPassword);
    #endif

    if (ConnectToNetwork(wifiSsid, wifiPassword)){
        UiFullClear();
        UiWarning("Updating...", "Checking for updates...");
        UiShow();
        if (CheckForUpdate()){
            OtaUpdate();
        }
        else{
            Serial.println("No update found.");
            UiFullClear();
            UiWarning("Update Failed", "No update found.");
            UiShow();
        }
    }
    else{
        Serial.println("Couldent connect to wifi, update failed.");
        UiFullClear();
        UiWarning("Update Failed", "Couldent connect to wifi network.");
        UiShow();
    }
    WaitForButtonPress();
    ESP.restart();
}