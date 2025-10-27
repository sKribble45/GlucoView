#include <Arduino.h>
#include "bg_datasource.h"
#include "config/config_manager.h"
#include <WiFi.h>
#include "wifi_manager.h"

// Data sources
#include "datasource_dexcom.h"
#include "datasource_nightscout.h"

BGDataSource dataSource;

GlucoseReading dsGl;
Config bgDSConfig;

double MgdlToMmol(int mgdl){
    return mgdl / 18.01559;
};

void UpdateDataSourceConfig(Config config){
    bgDSConfig = config;
}

bool BgDataSourceInit(){
    String dataSourceString = GetStringValue("data-source", bgDSConfig);
    // For people updating from previous version without nightscout support.
    if (dataSourceString == "" && GetStringValue("dex-username", bgDSConfig) != "" && GetStringValue("dex-password", bgDSConfig) != ""){
        dataSourceString = "dexcom";
    }

    if (dataSourceString == "nightscout"){
        dataSource = NIGHTSCOUT;
        String url = GetStringValue("ns-url", bgDSConfig);
        String secret = GetStringValue("ns-secret", bgDSConfig);
        NightscoutInit(url, secret);
        return true;
    }
    else{
        dataSource = DEXCOM;
        String username = GetStringValue("dex-username", bgDSConfig);
        String password = GetStringValue("dex-password", bgDSConfig);
        bool ous = GetBooleanValue("ous", bgDSConfig);
        return DexcomInit(username, password, ous);
    }
}

GlucoseReading GetRawBg(){
    GlucoseReading gl;
    if (dataSource == NIGHTSCOUT){
        gl = GetBgFromNightscout();
    }
    else if (dataSource == DEXCOM){
        gl =  GetBgFromDexcom();
    }
    return gl;
}

const int DEX_ERROR_ATTEMPTS = 2;
bool GetGl(){
    time_t currentTime;
    
    // Handle error
    for (int i = 0; i < DEX_ERROR_ATTEMPTS; i++){
        if (BgDataSourceInit()){
            dsGl = GetRawBg();
            if (!dsGl.failed){
                time(&currentTime);
                dsGl.minsSinceReading = round((currentTime - dsGl.timestamp) / 60);
                break;
            }
        }
        else{
            dsGl.failed = true;
        }

        if (dsGl.failed){
            // Reconnect to wifi network.
            WiFi.disconnect();
            ConnectToNetwork(
                GetStringValue("wifi-ssid", bgDSConfig),
                GetStringValue("wifi-password", bgDSConfig)
            );
        }
    }
    return !dsGl.failed;
}

GlucoseReading GlNow(){
    return dsGl;
}

void PrintGlucose(GlucoseReading gl){
    Serial.print("MMOL/L: ");
    Serial.println(gl.bg);
    Serial.print("MMOL/L Delta: ");
    Serial.println(gl.delta);
    Serial.print("MG/DL: ");
    Serial.println(gl.mgdl);
    Serial.print("MG/DL Delta: ");
    Serial.println(gl.mgdlDelta);
    Serial.print("Glucose Timestamp: ");
    Serial.print(gl.timestamp);
    Serial.print(", Tz adjusted: ");
    Serial.println(gl.tztimestamp);
    Serial.print("Trend: ");
    Serial.println(gl.trendDescription);
}