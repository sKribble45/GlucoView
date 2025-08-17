#include <Arduino.h>
#include <string>

using namespace std;

string RandomString(size_t length) {
    const string characterSet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/*+-=";
    string result;
    srand(esp_random());

    for (size_t i = 0; i < length; ++i) {
        result += characterSet[rand() % characterSet.size()];
    }
    return result;
}
String RemoveCharacterFromString(String input, char characterToRemove){
    String result;
    for (char chr : input){
        if (chr != characterToRemove){
            result += chr;
        }
    }
    return result;
};
