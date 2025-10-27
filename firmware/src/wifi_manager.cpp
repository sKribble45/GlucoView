#include <WiFi.h>
#include <WiFiUdp.h>
#include <list>
#include "wifi_manager.h"
#include <Arduino.h>
#include <unordered_map>
using namespace std;
const int WIFI_TIMEOUT = 10;

bool SavedNetworkExists(String ssid){
    WiFi.disconnect();
    // Scan for networks.
    Serial.println("Scanning for networks...");
    int foundNetworks = WiFi.scanNetworks();
    Serial.println("Finished scanning.");
    // if there are no networks found display no wifi and sleep.
    if (foundNetworks == 0){
        Serial.println("No wifi networks found :(");
        return false;
    }

    Serial.println("Networks Found:");
    for (int scannedNetworkIndex = 0; scannedNetworkIndex < foundNetworks; scannedNetworkIndex++){
        String scanned_network_ssid = WiFi.SSID(scannedNetworkIndex); // get the ssid from the index of a network that has has been scanned
        Serial.print(" -");
        Serial.println(scanned_network_ssid);


        // if the ssid from one of the saved networks equals one of the scanned networks then 
        if (ssid == scanned_network_ssid){
            Serial.print("Found saved network: ");
            Serial.println(ssid);
            return true;
        }
        
    }
    return false;
}

// Connects to a wifi network (returns 0 on sucess and the status on fail)
int ConnectToNetwork(String ssid, String password){
    // set wifi mode to station.
    WiFi.mode(WIFI_STA); 
    // disconnect from any existing network. (shouldent be neccicary)
    WiFi.disconnect(); 
    delay(500);
    // connect to a new network.
    WiFi.begin(ssid, password);

    
    int waitProgress = 0;
    Serial.print("Attempting to connect to ");
    Serial.println(ssid);

    Serial.println("Status [6: DISCONNECTED, 5: CONNECTION_LOST, 4: CONNECT_FAILED, 3: WL_CONNECTED, 2: SCAN_COMPLETED, 1: NO_SSID_AVAIL, 0: IDLE]: ");
    wl_status_t status;
    while (WiFi.status() != WL_CONNECTED && waitProgress <= (WIFI_TIMEOUT * 2)){
        status = WiFi.status();
        Serial.print(status);
        if (status == 4 || status == 5){
            break;
        }
        delay(500);
        waitProgress ++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED){
        Serial.print("Success! connected to ");
        Serial.println(ssid);
        return 0;
    }
    else{
        Serial.print("Failed to conect to network, wifi status: ");
        Serial.println(status);
        return status;
    }
}

int GetWifiSignalStrength(){
    int8_t wifiRSSI = WiFi.RSSI();
    if (wifiRSSI < -80){return 0;}
    else if (wifiRSSI < -70) {return 1;}
    else if (wifiRSSI < -60) {return 2;}
    else if (wifiRSSI < -30) {return 3;}
    else {return 0;}
}