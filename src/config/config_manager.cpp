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

void LoadConfig(UserConfig &config){
    Preferences prefs;
    prefs.begin(PREFERENCES_KEY, false);
    // Read all of the configuration values.
    config.wifiSsid = prefs.getString("wifi-ssid", "none");
    config.wifiPassword = prefs.getString("wifi-password", "none");
    config.dexcomUsername = prefs.getString("dex-username", "none");
    config.dexcomPassword = prefs.getString("dex-password", "none");
    config.twelveHourTime = prefs.getInt("twelve-hour-time", 10);
    // for (pair<string, ConfigValue> value : config){
    //     value.second.
    // }
    prefs.end();
}

bool ConfigExists(UserConfig config){
    // "none" is the value if it has failed to read the configuration values.
    if (config.wifiSsid == "none" ||
        config.wifiPassword == "none" ||
        config.dexcomUsername == "none" ||
        config.dexcomPassword == "none"){
        return false;
    }
    return true;
}


void SaveConfig(UserConfig config){
    Preferences prefs;
    prefs.begin(PREFERENCES_KEY, false);
    prefs.putString("wifi-ssid", config.wifiSsid);
    prefs.putString("wifi-password", config.wifiPassword);
    prefs.putString("dex-username", config.dexcomUsername);
    prefs.putString("dex-password", config.dexcomPassword);
    prefs.putInt("twelve-hour-time", config.twelveHourTime); // for some reason bool dosnt work :( so i have to use int.
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
void HostConfigAP(UserConfig &config,String APssid, String APpassword){
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
                                
                                // A checkbox by defult is ether "on" or "off" this converts it to a boolean (true or false)
                                bool twelveHourTime;
                                if (queryParsed["twelve_hour_time"] == "on"){twelveHourTime = true;}
                                else{twelveHourTime = false;}


                                config = {
                                    queryParsed["wifi_ssid"].c_str(),
                                    queryParsed["wifi_password"].c_str(),
                                    queryParsed["dexcom_username"].c_str(),
                                    queryParsed["dexcom_password"].c_str(),
                                    twelveHourTime
                                };
                                Serial.println("");

                                finishedConfig = true;

                                PrintFinishedHtml(client);
                            }
                            else{
                                PrintMainHtml(client);
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
    
    Serial.println("WiFi configured :)");
}