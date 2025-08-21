#include <Arduino.h>
#include "bg_datasource.h"
#include "config/config_manager.h"

// Data sources
#include "datasource_dexcom.h"
#include "datasource_nightscout.h"

BGDataSource dataSource;

GlucoseReading gl;

double MgdlToMmol(int mgdl){
    return mgdl / 18.01559;
};

bool BgDataSourceInit(Config config){
    String dataSourceString = GetStringValue("data-source", config);
    // For people updating from previous version without nightscout support.
    if (dataSourceString == "" && GetStringValue("dex-username", config) != "" && GetStringValue("dex-password", config) != ""){
        dataSourceString = "dexcom";
    }

    if (dataSourceString == "nightscout"){
        dataSource = NIGHTSCOUT;
        String url = GetStringValue("ns-url", config);
        String secret = GetStringValue("ns-secret", config);
        NightscoutInit(url, secret);
        return true;
    }
    else if (dataSourceString == "dexcom"){
        dataSource = DEXCOM;
        String username = GetStringValue("dex-username", config);
        String password = GetStringValue("dex-password", config);
        bool ous = GetBooleanValue("ous", config);
        bool success = DexcomInit(username, password, ous);
        return success;
    }
    else{
        return false;
    }
}

bool RetrieveGlDataSource(){
    if (dataSource == NIGHTSCOUT){
        gl = GetBgFromNightscout();
    }
    else if (dataSource == DEXCOM){
        gl = GetBgFromDexcom();
    }
    Serial.println(gl.failed);
    return !gl.failed;
}

GlucoseReading GlDataSource(){
    return gl;
}