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

WifiNetwork savedWifiNetwork = {"", ""};
DexcomAccount savedDexcomAccount = {"", ""};

const char *PREFERENCES_KEY = "levels-display";

void LoadConfig(){
    Preferences prefs;
    prefs.begin(PREFERENCES_KEY, false);
    savedWifiNetwork.ssid = prefs.getString("wifi-ssid", "none");
    savedWifiNetwork.password = prefs.getString("wifi-password", "none");
    savedDexcomAccount.username = prefs.getString("dex-username", "none");
    savedDexcomAccount.password = prefs.getString("dex-password", "none");
    prefs.end();
    Serial.println(savedWifiNetwork.ssid);
    Serial.println(savedWifiNetwork.password);
    Serial.println(savedDexcomAccount.username);
    Serial.println(savedDexcomAccount.password);
}

bool ConfigExists(){
    if (savedWifiNetwork.ssid == "none" || 
        savedWifiNetwork.password == "none" ||
        savedDexcomAccount.username == "none" || 
        savedDexcomAccount.password == "none"){
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
    savedDexcomAccount = {config.dexcomUsername, config.dexcomPassword};
    savedWifiNetwork = {config.wifiSsid, config.wifiPassword};
    
    prefs.end();
}
// vector<string> split(const string &s, char delim){
//     vector<string> result;
//     stringstream ss(s);
//     string item;

//     while (getline(ss, item, delim)) {
//         result.push_back(item);
//     }

//     return result;
// }

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

        WiFiClient client = server.available();   // Listen for incoming clients

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
                                string query = httpRequest.c_str();
                                query = query.substr(savePostIndex + 6);
                                query = query.substr(0, query.find(' '));
                                Serial.print("Query: ");
                                Serial.println(query.c_str());
                                
                                unordered_map<string, string> queryParsed = parseQueryString(query);

                                for (auto& p : queryParsed){
                                    Serial.print("[");
                                    Serial.print(p.first.c_str());
                                    Serial.print(", ");
                                    Serial.print(p.second.c_str());
                                    Serial.println("]");
                                }
                                
                                config = { 
                                    queryParsed["wifi_ssid"].c_str(),
                                    queryParsed["wifi_password"].c_str(),
                                    queryParsed["dexcom_username"].c_str(),
                                    queryParsed["dexcom_password"].c_str()
                                };
                                finishedConfig = true;
                            }

                            //// int savePostIndex = httpRequest.indexOf("POST /save/");
                            //// if (savePostIndex >= 0) {
                            ////     String ssid;
                            ////     String password;
                            ////     String filteredHttpRequest = httpRequest.substring(0, savePostIndex + 11);
                            ////     Serial.print("filtered http request: ");
                            ////     Serial.println(filteredHttpRequest);

                            ////     vector<string> splitRequest = split(filteredHttpRequest.c_str(), '/');
                            ////     if (splitRequest[0] != "" && splitRequest[1] != "") {
                            ////         config.wifi = true;
                            ////         config.wifiSsid = splitRequest[0].c_str();
                            ////         config.wifiPassword = splitRequest[1].c_str();
                            ////     }else{
                            ////         config.wifi = false;
                            ////     }

                            ////     if (splitRequest[2] != "" && splitRequest[3] != "") {
                            ////         config.dexcom = true;
                            ////         config.dexcomUsername = splitRequest[0].c_str();
                            ////         config.dexcomPassword = splitRequest[1].c_str();
                            ////     }else{
                            ////         config.dexcom = false;
                            ////     }
                            ////     finishedConfig = true;
                            //// }
                            //
                            //// int wifiRequestIndex = httpRequest.indexOf("POST /set_wifi/");
                            //// if (wifiRequestIndex >= 0) {
                            ////     String ssid;
                            ////     String password;
                            ////     bool write_ssid = true;

                            ////     for (int i = 15; httpRequest[i] != ' '; i++){
                            ////         if (httpRequest[i] + wifiRequestIndex == '/'){
                            ////             write_ssid = false;
                            ////         }
                            ////         else{
                            ////             if (write_ssid){
                            ////                 ssid += httpRequest[i];
                            ////             }
                            ////             else{
                            ////                 password += httpRequest[i];
                            ////             }
                            ////         }
                            ////     }
                            ////     config.wifi = true;
                            ////     config.wifiSsid = ssid;
                            ////     config.wifiPassword = password;
                            //// }

                            //// int dexcomRequestPost = httpRequest.indexOf("POST /set_dexcom/");
                            //// if (dexcomRequestPost >= 0) {
                            ////     String username;
                            ////     String password;
                            ////     bool write_username = true;

                            ////     for (int i = 17; httpRequest[i] != ' '; i++){
                            ////         if (httpRequest[i] + dexcomRequestPost == '/'){
                            ////             write_username = false;
                            ////         }
                            ////         else{
                            ////             if (write_username){
                            ////                 username += httpRequest[i];
                            ////             }
                            ////             else{
                            ////                 password += httpRequest[i];
                            ////             }
                            ////         }
                            ////     }
                            ////     config.dexcom = true;
                            ////     config.dexcomUsername = username;
                            ////     config.dexcomPassword = password;
                            //// }

                            //// if (httpRequest.indexOf("POST /done") >= 0) {
                            ////     finishedConfig = true;
                            //// }

                            PrintHtml(client);
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