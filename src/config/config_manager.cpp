#include <WiFi.h>
#include <Arduino.h>
#include "config_manager.h"
#include "config_gui_html.h"
#include <Preferences.h>
#include "wifi_manager/wifi_manager.h"
#include "dexcom/dexcom_account.h"
#include "UI.h"
#include <vector>
#include <sstream>
#include <unordered_map>
using namespace std;

const char *PREFERENCES_KEY = "glucoview";
const char *SERIAL_NUMBER_KEY = "serialnum";

// Edit this when adding a new configuration value along with the html.
Config CONFIG_TEMPLATE = {
    {"wifi-ssid", ""},
    {"wifi-password", ""},
    {"dex-username", ""},
    {"dex-password", ""},
    {"12h-time", false}
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
unordered_map<string,string> parseQueryString(const string& query) {
    unordered_map<string, string> params;
    
    string key, value;
    stringstream ss(query);
    string pair;

    while (getline(ss, pair, '&')) {
        size_t equal_pos = pair.find('=');
        if (equal_pos != string::npos) {
            key = pair.substr(0, equal_pos);
            value = pair.substr(equal_pos + 1);
            params[key] = value;
        }
    }

    return params;
}


WiFiServer server(80);
void HostConfigAP(Config &config,String APssid, String APpassword){
    String httpRequest;
    bool finishedConfig = false;

    WiFi.softAP(APssid, APpassword, 1, 0, 1);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    server.begin();

    int connectedClients = 0;
    int prevConnectedClients = 1;

    while (!finishedConfig){
        connectedClients = WiFi.softAPgetStationNum();
        if (connectedClients >= 1 && prevConnectedClients < 1){
            String link = "http://" + IP.toString() + "/";
            UiFullClear();
            UiWebPageConectionPage(link);
            UiShow();
        }
        else if(connectedClients < 1 && prevConnectedClients >= 1){
            UiFullClear();
            UiWiFiConectionPage(APssid, APpassword);
            UiShow();
        }
        prevConnectedClients = connectedClients;

        WiFiClient client = server.accept();   // Listen for incoming clients

        if (client) {
            Serial.println("New Client.");
            String currentLine = "";

            while (client.connected()) {
                if (client.available()) {
                    char c = client.read();
                    httpRequest += c;
                    if (c == '\n') {
                        // if the current line is blank, you got two newline characters in a row.
                        // that's the end of the client HTTP request, so send a response:
                        if (currentLine.length() == 0) {
                            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                            // and a content-type so the client knows what's coming, then a blank line:
                            client.println("HTTP/1.1 200 OK");
                            client.println("Content-type:text/html");
                            client.println("Connection: close");
                            client.println();
                            
                            Serial.println(httpRequest);
                            int savePostIndex = httpRequest.indexOf("GET /?");
                            if (savePostIndex >= 0) {
                                // Get the configuration section of the http request e.g. "foo=bar, blah=on"
                                string query = httpRequest.c_str();
                                query = query.substr(savePostIndex + 6);
                                query = query.substr(0, query.find(' '));

                                // print the query for debugging.
                                Serial.print("Query: ");
                                Serial.println(query.c_str());
                                
                                // Convert the http request to a unordered map.
                                unordered_map<string, string> queryParsed = parseQueryString(query);

                                for (auto& p : queryParsed){
                                    Serial.print("[");
                                    Serial.print(p.first.c_str());
                                    Serial.print(", ");
                                    Serial.print(p.second.c_str());
                                    Serial.println("]");
                                }
                                
                                for (pair<string, string> queryEntry : queryParsed){
                                    // If the name of the query entry e.g. "wifi-ssid" is in the template and what variable type it corosponds to.
                                    if (CONFIG_TEMPLATE.contains(queryEntry.first)){
                                        // If its an int, convert the string recieved into a intager. (same thing for the rest)
                                        if (holds_alternative<int>(CONFIG_TEMPLATE[queryEntry.first])){
                                            config[queryEntry.first] = stoi(queryEntry.second);
                                        }
                                        if (holds_alternative<String>(CONFIG_TEMPLATE[queryEntry.first])){
                                            config[queryEntry.first] = queryEntry.second.c_str(); // convert to a char string to conver to String from std::string
                                        }
                                        if (holds_alternative<double>(CONFIG_TEMPLATE[queryEntry.first])){
                                            config[queryEntry.first] = stod(queryEntry.second);
                                        }
                                        if (holds_alternative<bool>(CONFIG_TEMPLATE[queryEntry.first])){
                                            // A checkbox in html has 2 states (on or off) this converts them into a boolean (true or false)
                                            bool value;
                                            if (queryEntry.second == "on"){value = true;}
                                            else{value = false;}
                                            config[queryEntry.first] = value;
                                        }
                                    }
                                    
                                }

                                finishedConfig = true;

                                PrintFinishedHtml(client);
                            }
                            else{
                                PrintMainHtml(client, config);
                            }

                            
                            // The HTTP response ends with another blank line
                            client.println();
                            // Break out of the while loop
                            break;
                        } else { // if you got a newline, then clear currentLine
                            currentLine = "";
                        }
                    } else if (c != '\r') {  // if you got anything else but a carriage return character,
                        currentLine += c;      // add it to the end of the currentLine
                    }
                }
            }
            // Clear the httpRequest variable
            httpRequest = "";
            // Close the connection
            client.stop();
            Serial.println("Client disconnected.");
            Serial.println("");
        }
    }
    server.end();
    
    Serial.println("Configured :)");
}

string RandomString(size_t length) {
    const string characterSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/*+-=";
    string result;
    // Set random seed to the time.
    // TODO: make this not time based.
    srand(time(0));

    for (size_t i = 0; i < length; ++i) {
        result += characterSet[rand() % characterSet.size()];
    }
    return result;
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