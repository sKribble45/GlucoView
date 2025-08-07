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

using namespace std;

const char* repo = "sKribble45/GlucoView";

RTC_DATA_ATTR bool updateNeeded = false;
RTC_DATA_ATTR time_t lastCheckedForUpdates = 0;

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

// Get version from github (defults to 0.0.0)
Version GetVersion(){
    JsonDocument json;
    if (GetReleases(json, repo)){
        String versionNameUnfiltered = json[0]["tag_name"].as<const char*>();
        String versionName;        
        for (char chr : versionNameUnfiltered){
            if (chr != 'V' && chr != 'v'){
                versionName += chr;
            }
        }

        Version version;

        stringstream ss(versionName.c_str());
        string segment;
        // Extract major
        if (getline(ss, segment, '.')) {
            version.major = std::stoi(segment);
        }
        // Extract minor
        if (getline(ss, segment, '.')) {
            version.minor = std::stoi(segment);
        }
        // Extract revision
        if (getline(ss, segment, '.')) {
            version.revision = std::stoi(segment);
        }
        
        return version;
    }
    else{
        return {0,0,0};
    }
    
}

bool CheckForUpdates(){
    time_t t = time(0);
    if (t - lastCheckedForUpdates > 30*60 || lastCheckedForUpdates == 0){
        Version latestVersion = GetVersion();
        if (latestVersion.major != 0){
            updateNeeded = (latestVersion.major > VERSION_MAJOR || latestVersion.minor > VERSION_MINOR || latestVersion.revision > VERSION_REVISION);
            lastCheckedForUpdates = t;
        }
    }
    return updateNeeded;
}