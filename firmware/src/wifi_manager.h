#include <list>
#include <Arduino.h>
using namespace std;


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __WIFI_MANAGER_H
#define __WIFI_MANAGER_H

bool SavedNetworkExists(String ssid);
int ConnectToNetwork(String ssid, String password);
int GetWifiSignalStrength();

#endif /* __WIFI_MANAGER_H */