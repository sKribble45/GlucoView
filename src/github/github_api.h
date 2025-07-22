#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#ifndef __GITHUB_API_H
#define __GITHUB_API_H

bool GetReleases(JsonDocument &json, String repo);

#endif