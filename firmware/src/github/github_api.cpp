#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>


bool GetReleases(JsonDocument &json, String repo){
    HTTPClient httpClient;

    httpClient.begin("https://api.github.com/repos/"+repo+"/tags");

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