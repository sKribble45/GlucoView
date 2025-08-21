#include <Arduino.h>
#include "config/config_manager.h"
#include <WebServer.h>
#include <WiFi.h>
#include "config/config_gui_html.h"
#include <sstream>
#include "UI.h"

using namespace std;

WebServer server(80);
Config tmpConfig;
bool finished = false;


// Web server functions

void OnRoot(){
    server.send(200, "text/html", GetMainHtml(tmpConfig));
}

void OnSave(){
    finished = true;
    // Convert the http request to a unordered map.
    for (int arg = 0; arg < server.args(); arg++){
        // If the name of the query entry e.g. "wifi-ssid" is in the template and what variable type it corosponds to.
        string argName = server.argName(arg).c_str();
        string argValue = server.arg(arg).c_str();
        if (CONFIG_TEMPLATE.contains(argName)){
            // If its an int, convert the string recieved into a intager. (same thing for the rest)
            if (holds_alternative<int>(CONFIG_TEMPLATE[argName])){
                tmpConfig[argName] = stoi(argValue);
            }
            if (holds_alternative<String>(CONFIG_TEMPLATE[argName])){
                String queryStringValue = argValue.c_str();
                if (queryStringValue != MaskPassword(get<String>(tmpConfig[argName]))){
                    tmpConfig[argName] = queryStringValue;
                }
            }
            if (holds_alternative<double>(CONFIG_TEMPLATE[argName])){
                tmpConfig[argName] = stod(argValue);
            }
            if (holds_alternative<bool>(CONFIG_TEMPLATE[argName])){
                // A checkbox in html has 2 states (on or off) this converts them into a boolean (true or false)
                bool value;
                if (argValue == "on"){value = true;}
                else{value = false;}
                tmpConfig[argName] = value;
            }
        }
    }
    server.send(200, "text/html", GetFinishedHtml());
}

void HostConfigAP(Config &config, String APssid, String APpassword){
    tmpConfig = config;

    WiFi.softAP(APssid, APpassword, 1, 0, 1);
    String IP = WiFi.softAPIP().toString();

    server.on("/", OnRoot);
    server.on("/save", HTTP_POST, OnSave);

    // Start the web server.
    server.begin();
    
    int connectedClients = 0;
    int prevConnectedClients = 1;
    while (!finished){
        connectedClients = WiFi.softAPgetStationNum();
        if (connectedClients >= 1 && prevConnectedClients < 1){
            String link = "http://" + IP + "/";
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
        server.handleClient();
    }
    
    config = tmpConfig;
}
