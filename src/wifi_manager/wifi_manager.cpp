#include <WiFi.h>
#include <WiFiUdp.h>
#include <list>
#include "wifi_manager.h"
#include <Arduino.h>
#include <unordered_map>
using namespace std;


bool SavedNetworkExists(WifiNetwork network){
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
        if (network.ssid == scanned_network_ssid){
            Serial.print("Found saved network: ");
            Serial.println(network.ssid);
            return true;
        }
        
    }
    return false;
}

WifiNetwork GetSavedNetwork(list<WifiNetwork> saved_networks){
    WiFi.disconnect();
    // Scan for networks.
    Serial.println("Scanning for networks...");
    int foundNetworks = WiFi.scanNetworks();
    Serial.println("Finished scanning.");
    // if there are no networks found display no wifi and sleep.
    if (foundNetworks == 0){
        Serial.println("No wifi networks found :(");
        return {"",""};
    }

    bool savedNetworkExists = false;
    WifiNetwork finalSavedNetwork = {"", ""};


    Serial.println("Networks Found:");
    for (int scannedNetworkIndex = 0; scannedNetworkIndex < foundNetworks; scannedNetworkIndex++){
        String scanned_network_ssid = WiFi.SSID(scannedNetworkIndex); // get the ssid from the index of a network that has has been scanned
        
        Serial.print(" -");
        Serial.print(scanned_network_ssid);
        Serial.print(" ");
        Serial.println(WiFi.RSSI(scannedNetworkIndex));

        for (WifiNetwork saved_network : saved_networks){
            // nicer variable names cuz i feel like it
            String ssid = saved_network.ssid;
            String pass = saved_network.password;
            // if the ssid from one of the saved networks equals one of the scanned networks then 
            if (ssid == scanned_network_ssid){
                savedNetworkExists = true;
                finalSavedNetwork.ssid = scanned_network_ssid;
                finalSavedNetwork.password = pass;
                break;
            }
        }
        if (savedNetworkExists){
            Serial.print("Found saved network: ");
            Serial.println(finalSavedNetwork.ssid);
            break;
        }
    }
    WiFi.scanDelete();
    return finalSavedNetwork;
    
}

bool ConnectToNetwork(String ssid, String password){
    
    // connect to the found wifi network.
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(500);
    WiFi.begin(ssid, password);

    int waiTimeLimitS = 15;
    int waitProgress = 0;
    Serial.print("Attempting to connect to ");
    Serial.println(ssid);

    Serial.println("Status [6: DISCONNECTED, 5: CONNECTION_LOST, 4: CONNECT_FAILED, 3: WL_CONNECTED, 2: SCAN_COMPLETED, 1: NO_SSID_AVAIL, 0: IDLE]: ");
    while (WiFi.status() != WL_CONNECTED && waitProgress <= (waiTimeLimitS * 2)){
        Serial.print(WiFi.status());
        delay(500);
        waitProgress ++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED){
        Serial.print("Success! connected to ");
        Serial.println(ssid);
        return true;
    }
    else{
        Serial.println("Failed to conect to network. :(");
        return false;
    }
}


