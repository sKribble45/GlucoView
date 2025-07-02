#include <Arduino.h>
#include "wifi_manager/wifi_manager.h"
#include "dexcom/dexcom_account.h"

#ifndef __CONFIG_MANAGER_H
#define __CONFIG_MANAGER_H

typedef struct {
    String wifiSsid;
    String wifiPassword;
    String dexcomUsername;
    String dexcomPassword;
    bool twelveHourTime;
} UserConfig;


extern RTC_DATA_ATTR UserConfig savedConfig;

void LoadConfig();
bool ConfigExists();
void SaveConfig(UserConfig config);
void HostConfigAP(UserConfig &config,String APssid, String APpassword);

#endif