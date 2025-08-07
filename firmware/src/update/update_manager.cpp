#include "github/github_api.h"
#include "ArduinoJson.h"
#include "update_manager.h"
#include <vector>
using namespace std;

const char* repo = "sKribble45/GlucoView";

RTC_DATA_ATTR bool updateNeeded = false;
RTC_DATA_ATTR time_t lastCheckedForUpdates = 0;

vector<String> SplitString(String str, char split){
    int targetIndex = 0;
    vector<String> output;
    for (char chr : str){
        if (chr != split){
            output[targetIndex] += chr;
        }
        else{targetIndex ++;}
    }
    return output;
}

// Get version from github (defults to 0.0.0)
Version GetVersion(){
    JsonDocument json;
    if (GetReleases(json, repo)){
        String versionNameUnfiltered = json[0]["name"].as<const char*>();
        String versionName;        
        for (char chr : versionNameUnfiltered){
            if (chr != 'V' && chr != 'v'){
                versionName += chr;
            }
        }
        vector<String> versionStrList = SplitString(versionName, '.');
        Version version;
        version.major = stoi(versionStrList[0].c_str());
        version.minor = stoi(versionStrList[1].c_str());
        version.revision = stoi(versionStrList[2].c_str());
        return version;
    }
    else{
        return {0,0,0};
    }
    
}

bool CheckForUpdates(){
    time_t t = time(0);
    if (t - lastCheckedForUpdates > 30*60 || lastCheckedForUpdates == 0){
        Version latestVersion = GetVersion();
        if (latestVersion.major != 0){
            updateNeeded = (latestVersion.major > VERSION_MAJOR || latestVersion.minor > VERSION_MINOR || latestVersion.revision > VERSION_REVISION);
            lastCheckedForUpdates = t;
        }
    }
    return updateNeeded;
}

