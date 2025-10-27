#include <Arduino.h>
#include <HTTPClient.h>
#include "util.h"
#include "glucose/bg_datasource.h"
#include <ArduinoJson.h>
#include <string>
using namespace std;

// Dexcom api base urls.
const char *DEXCOM_BASE_URL = "https://share2.dexcom.com/ShareWebServices/Services";
const char *DEXCOM_BASE_URL_OUS = "https://shareous1.dexcom.com/ShareWebServices/Services";
const char *DEXCOM_APPLICATION_ID = "d8665ade-9673-4e27-9ff6-92db4ce13d13";
char DEXCOM_DEFAULT_SESSION_ID[37] = "00000000-0000-0000-0000-000000000000";

// Dexcom credentials.
String dexUsername;
String dexPassword;
bool dexOus;
String dexServerUrl;

RTC_DATA_ATTR char sessionID[37] = "00000000-0000-0000-0000-000000000000";
RTC_DATA_ATTR unsigned long lastSessionIdRenewal = 0;
const unsigned long sessionIDTimout = 2*60;

// Get session id from dexcom (returns 00000000-0000-0000-0000-000000000000 if failed)
void UpdateDexSessionId(String username, String password, String url){
    // Make the json string containing the application id, username and password.
    String jsonString;
    jsonString += "{";
    jsonString += "\"accountName\": \"" + username + "\",";
    jsonString += "\"applicationId\": \"" + String(DEXCOM_APPLICATION_ID) + "\",";
    jsonString += "\"password\": \"" + password + "\"";
    jsonString += "}";

    // Start the http client and post the json.
    HTTPClient http;
    http.begin(url + "/General/LoginPublisherAccountByName");
    http.addHeader("Content-Type", "application/json");
    int httpCode = http.POST(jsonString);

    if (httpCode == HTTP_CODE_OK){
        char session[37];
        RemoveCharacterFromString(http.getString(), '\"').toCharArray(session, 37, 0);
        strcpy(sessionID, session);
    }
    else{
        strcpy(sessionID, DEXCOM_DEFAULT_SESSION_ID);
    }
}

bool DexcomInit(String username, String password, bool ous){
    Serial.println("Initilise dexcom");
    Serial.print("Session ID: ");
    Serial.println(sessionID);
    dexOus = ous;
    dexUsername = username;
    dexPassword = password;
    if (ous){dexServerUrl = DEXCOM_BASE_URL_OUS;}
    else{dexServerUrl = DEXCOM_BASE_URL;}
    time_t currentTime;
    time(&currentTime);
    // If its been longer than the timeout time since the last time the session id was generated.
    if ((unsigned long)currentTime - lastSessionIdRenewal > sessionIDTimout || strcmp(sessionID, DEXCOM_DEFAULT_SESSION_ID) == 0){
        Serial.print("Updating session ID, New session ID: ");
        UpdateDexSessionId(dexUsername, dexPassword, dexServerUrl);
        lastSessionIdRenewal = currentTime;
        Serial.println(sessionID);
    }
    if (sessionID == DEXCOM_DEFAULT_SESSION_ID){
        return false;
    }
    else{
        return true;
    }
}

GlucoseReading GetBgFromDexcom(){
    Serial.print("Session ID: ");
    Serial.println(sessionID);

    HTTPClient http;
    String url = dexServerUrl + "/Publisher/ReadPublisherLatestGlucoseValues?sessionId=";
    url += sessionID;
    url += "&minutes=1440&maxCount=2";
    http.begin(url);

    int httpCode = http.GET();
    GlucoseReading gl;
    gl.failed = true;

    if (httpCode == HTTP_CODE_OK){
        JsonDocument json;
        String response = http.getString();
        Serial.println(response);
        DeserializationError error = deserializeJson(json, response);
        if (error){
            Serial.print("Error parsing JSON: ");
            Serial.println(error.c_str());
            return gl;
        }

        // Glucose values
        gl.mgdl = json[0]["Value"].as<int>();
        gl.mgdlDelta = json[0]["Value"].as<int>() - json[1]["Value"].as<int>();

        gl.bg = MgdlToMmol(gl.mgdl);
        gl.delta = MgdlToMmol(gl.mgdlDelta);
        
        json[0]["Trend"].as<String>().toCharArray(gl.trendDescription, sizeof(gl.trendDescription));

        string dtUnfiltered = json[0]["DT"].as<string>();
        // Cut out the numerical part from the "Date()" wrapper.
        dtUnfiltered = dtUnfiltered.substr(5, dtUnfiltered.length()-2); // start at pos 5 ("Date(" <- here ) and create a string from there to before the last character (")").
        string timestampMilisStr = dtUnfiltered.substr(0, dtUnfiltered.length()-6);
        // Convert miliseconds to seconds for unix timestamp.
        gl.timestamp = stoi(timestampMilisStr.substr(0, timestampMilisStr.length()-3));

        // Get the utc offset string formated hhmm (h: hour, m: min)
        string utcOffsetHrMinStr = dtUnfiltered.substr(dtUnfiltered.length()-5, dtUnfiltered.length());
        // Split utc offset string into mins and hours.
        string utcOffsetHrStr = utcOffsetHrMinStr.substr(0, 2);
        string utcOffsetMinStr = utcOffsetHrMinStr.substr(2, 4);
        // Calculate utc offset.
        int utcOffset = (stoi(utcOffsetHrStr) * 60 * 60) + (stoi(utcOffsetMinStr) * 60);
        gl.tztimestamp = gl.timestamp + utcOffset;

        gl.failed = false;
    }
    return gl;
}