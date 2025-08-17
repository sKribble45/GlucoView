#include <Arduino.h>
#include "..\wifi_manager.h"
#include <unordered_map>
#include <variant>
#include <string>
using namespace std;

#ifndef __CONFIG_MANAGER_H
#define __CONFIG_MANAGER_H


typedef variant<int, double, String, bool> ConfigValue;

typedef unordered_map<string, ConfigValue> Config;

void PrintConfigValues(Config config);
void LoadConfig(Config &config);
bool ConfigExists(Config config);
void SaveConfig(Config config);
void HostConfigAP(Config &config,String APssid, String APpassword);
string RandomString(size_t length);
String GetSerialNumber();
bool SerialNumberExists();
void RandomiseSerialNumber();
void ResetSettings();

String GetStringValue(string value, Config config);
int GetIntValue(string value, Config config);
double GetDoubleValue(string value, Config config);
bool GetBooleanValue(string value, Config config);

#endif