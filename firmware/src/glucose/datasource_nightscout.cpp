#include <Arduino.h>
#include <SHA1Builder.h>
#include <HTTPClient.h>
#include "glucose/bg_datasource.h"
#include <ArduinoJson.h>
// Nightscout credentials.
String nsURL;
String hashedNsSecret;

void NightscoutInit(String URL, String secret){
    nsURL = URL;
    // Sha1 encode the secret as the rest api only accepts it hashed.
    SHA1Builder sha;
    sha.begin();
    sha.add(secret);
    sha.calculate();
    hashedNsSecret = sha.toString();
}

GlucoseReading GetBgFromNightscout(){
    HTTPClient http;

    String url = nsURL + "api/v1/entries.json?count=1";
    Serial.print("URL: ");
    Serial.println(url);
    http.begin(url);

    http.addHeader("api-secret", hashedNsSecret);
    int httpCode = http.GET();
    GlucoseReading gl;
    gl.failed = true;

    if (httpCode == HTTP_CODE_OK){
        String response = http.getString();
        Serial.println(response);

        JsonDocument json;
        DeserializationError error = deserializeJson(json, response);
        if (error){
            Serial.print("Error parsing JSON: ");
            Serial.println(error.c_str());
            return gl;
        }

        // Glucose values
        gl.mgdl = json[0]["sgv"].as<int>();
        gl.mgdlDelta = json[0]["delta"].as<int>();
        gl.bg = MgdlToMmol(json[0]["sgv"].as<int>());
        gl.delta = MgdlToMmol(round(json[0]["delta"].as<double>()));
        string dtStr = json[0]["date"].as<string>();
        gl.timestamp = stoi(dtStr.substr(0, dtStr.length()-3));
        gl.tztimestamp = gl.timestamp + (json[0]["utcOffset"].as<int>()*60);
        gl.trend_description = json[0]["direction"].as<String>();
        gl.failed = false;
    }
    return gl;
}