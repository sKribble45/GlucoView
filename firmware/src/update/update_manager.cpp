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

bool CheckForUpdate(){
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

String GetFirmwareUrl(){
    String URL = "";
    JsonDocument json;
    if (GetReleases(json, repo)){
        for (JsonObject asset : json[0]["assets"].as<JsonArray>()){
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