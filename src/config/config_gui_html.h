#include <WiFi.h>

void PrintMainHtml(WiFiClient &client, Config config);
void PrintFinishedHtml(WiFiClient &client);

String MaskPassword(String password);