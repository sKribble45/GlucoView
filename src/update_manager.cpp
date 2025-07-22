#include "github/github_api.h"
#include "ArduinoJson.h"
#include "update_manager.h"

using namespace std;

const char* repo = "sKribble45/Gluco-View";

RTC_DATA_ATTR bool updateNeeded = false;
RTC_DATA_ATTR time_t lastCheckedForUpdates = 0;

// Get version from github (defults to 0.0)
double GetVersion(){
    JsonDocument json;
    if (GetReleases(json, repo)){
        String versionNameUnfiltered = json[0]["name"].as<const char*>();
        String versionName;
        for (char chr : versionNameUnfiltered){
            if (chr != 'V' && chr != 'v'){
                versionName += chr;
            }
        }

        double versionNum = atof(versionName.c_str());
        return versionNum;
    }
    else{
        return 0.0;
    }
}

bool CheckForUpdates(){
    time_t t = time(0);
    if (t - lastCheckedForUpdates > 30*60 || lastCheckedForUpdates == 0){
        double latestVersion = GetVersion();
        if (latestVersion != 0.0){
            updateNeeded = (latestVersion > VERSION);
            lastCheckedForUpdates = t;
        }
    }
    return updateNeeded;
}

