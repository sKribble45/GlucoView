#include <list>
#include <Arduino.h>
using namespace std;


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __WIFI_MANAGER_H
#define __WIFI_MANAGER_H

typedef struct {
    String ssid;
    String password;
} WifiNetwork;



WifiNetwork GetSavedNetwork(list<WifiNetwork> saved_networks);
bool SavedNetworkExists(WifiNetwork network);
bool ConnectToNetwork(String ssid, String password);
int GetSignalStrength();

#endif /* __WIFI_MANAGER_H */