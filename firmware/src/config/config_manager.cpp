#include <WiFi.h>
#include <Arduino.h>
#include "config_manager.h"
#include "config_gui_html.h"
#include <Preferences.h>
#include "wifi_manager.h"
#include "UI.h"
#include <vector>
#include <sstream>
#include <unordered_map>
#include "util.h"

using namespace std;

const char *PREFERENCES_KEY = "glucoview";
const char *SERIAL_NUMBER_KEY = "serialnum";

// Edit this when adding a new configuration value along with the html.
Config CONFIG_TEMPLATE = {
    // WiFi:
    {"wifi-ssid", ""},
    {"wifi-password", ""},

    {"data-source", ""},

    // Dexcom:
    {"dex-username", ""},
    {"dex-password", ""},
    {"ous", true},
    // Nightscout:
    {"ns-url", ""},
    {"ns-secret", ""},

    {"mmol-l", true},

    // Ui Options:
    {"12h-time", false},
    {"trend-arrow", true},
    {"delta", true},
    {"timestamp", true},
    {"rel-timestamp", true},
    {"wifi-icon", true},

    // Update Options:
    {"update-check", true},
    {"auto-update", true},
    {"beta", false}
};

void PrintConfigValues(Config config){
    Serial.print("{");
    // Loop through all entries.
    for (auto& p : config){
        Serial.print("{");
        Serial.print(p.first.c_str());
        Serial.print(", ");
        // Prints the value.
        if (holds_alternative<String>(p.second)){
            Serial.print(get<String>(p.second).c_str());
        }
        else if (holds_alternative<int>(p.second)){
            Serial.print(get<int>(p.second));
        }
        else if (holds_alternative<double>(p.second)){
            Serial.print(get<double>(p.second));
        }
        else if (holds_alternative<bool>(p.second)){
            Serial.print(get<bool>(p.second));
        }
        Serial.print("}, ");
    }
    Serial.println("}");
}

void LoadConfig(Config &config){
    Preferences prefs;
    prefs.begin(PREFERENCES_KEY, false);
    // Read all of the configuration values.
    for (pair<string, ConfigValue> templateEntry : CONFIG_TEMPLATE){
        // if the value is of a certain type in the template read that type from storage and write it to the config in the same location.
        if (holds_alternative<int>(templateEntry.second)){
            // "get<int>(templateEntry.second)" uses the value from the template as the value returned after it has failed to retrieve the data from storage.
            config[templateEntry.first] = prefs.getInt(templateEntry.first.c_str(), get<int>(templateEntry.second));
        }
        else if (holds_alternative<String>(templateEntry.second)){
            config[templateEntry.first] = prefs.getString(templateEntry.first.c_str(), get<String>(templateEntry.second));
        }
        else if (holds_alternative<double>(templateEntry.second)){
            config[templateEntry.first] = prefs.getDouble(templateEntry.first.c_str(), get<double>(templateEntry.second));
        }
        else if (holds_alternative<bool>(templateEntry.second)){
            config[templateEntry.first] = (bool)prefs.getInt(templateEntry.first.c_str(), get<bool>(templateEntry.second));
        }
    }
    
    prefs.end();
}

bool ConfigExists(Config config){
    if (config.size() <= 0){return false;}

    int numConfigValues = CONFIG_TEMPLATE.size();
    int configValuesEquivelentToTemplate = 0;
    for (pair<string, ConfigValue> templateEntry : CONFIG_TEMPLATE){
        // if the value in the config is the same as the template add one to the counter checking how many. 
        //(counter is required just in case the value in config exists just is set to the same value in the template)
        if (config[templateEntry.first] == templateEntry.second){
            configValuesEquivelentToTemplate ++;
        }
    }
    if (configValuesEquivelentToTemplate == numConfigValues){return false;}
    else{return true;}
}


void SaveConfig(Config config){
    Preferences prefs;
    prefs.begin(PREFERENCES_KEY, false);
    // prefs.clear();
    for (pair<string, ConfigValue> configEntry : config){
        if (holds_alternative<int>(configEntry.second)){
            prefs.putInt(configEntry.first.c_str(), get<int>(configEntry.second));
        }
        else if (holds_alternative<String>(configEntry.second)){
            prefs.putString(configEntry.first.c_str(), get<String>(configEntry.second));
        }
        else if (holds_alternative<double>(configEntry.second)){
            prefs.putDouble(configEntry.first.c_str(), get<double>(configEntry.second));
        }
        else if (holds_alternative<bool>(configEntry.second)){ // Boolean dosnt work with saving:( no idea why so i used int.
            prefs.putInt(configEntry.first.c_str(), get<bool>(configEntry.second));
        }
    }


    prefs.end();
}

String GetStringValue(string key, Config config){
    ConfigValue configValue = config[key];
    if (holds_alternative<String>(configValue)){
        return get<String>(configValue);
    }
    else{return "";}
}
int GetIntValue(string key, Config config){
    ConfigValue configValue = config[key];
    if (holds_alternative<int>(configValue)){
        return get<int>(configValue);
    }
    else{return 0;}
}
double GetDoubleValue(string key, Config config){
    ConfigValue configValue = config[key];
    if (holds_alternative<double>(configValue)){
        return get<double>(configValue);
    }
    else{return 0.0;}
}
bool GetBooleanValue(string key, Config config){
    ConfigValue configValue = config[key];
    if (holds_alternative<bool>(configValue)){
        return get<bool>(configValue);
    }
    else{return false;}
}

void RandomiseSerialNumber(){
    Preferences prefs;
    prefs.begin(SERIAL_NUMBER_KEY, false);
    prefs.putString("serial-number", RandomString(4).c_str());
    prefs.end();
}

bool SerialNumberExists(){
    Preferences prefs;
    prefs.begin(SERIAL_NUMBER_KEY, true);
    if (prefs.getString("serial-number", "") != ""){
        return true;
    }
    else{
        return false;
    }
    prefs.end();
}

String GetSerialNumber(){
    Preferences prefs;
    prefs.begin(SERIAL_NUMBER_KEY, true);
    String serialNumber = prefs.getString("serial-number", "");
    return serialNumber;
}

void ResetSettings(){
    SaveConfig(CONFIG_TEMPLATE);
}