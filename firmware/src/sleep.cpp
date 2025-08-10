#include <WiFi.h>
#include <Arduino.h>
#include "epaper/EPD.h"
#include "driver/rtc_io.h"


void Sleep(int sleep_seconds){
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.print("Going to sleep for ");
    Serial.print(sleep_seconds);
    Serial.println("s *yawn*");
    EPD_2in13_V4_Sleep(); // tell the screen to go to sleep "good night" *yawn*
    esp_sleep_enable_timer_wakeup(sleep_seconds * 1000000); // tel the esp how long to sleep *big yawn*
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, 1);
    rtc_gpio_pullup_dis((gpio_num_t)BUTTON_PIN);
    rtc_gpio_pulldown_en((gpio_num_t)BUTTON_PIN);
    //esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF); // very sleep deep *strech*
    esp_deep_sleep_start(); // COMENSE THE SLEEP!
}