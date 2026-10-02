#pragma once

#include <Arduino.h>

#define PROJECT_NAME "BareMetal"
#define SHOW_WIFI_SETTINGS 0

#include <GyverDBFile.h>
#include <SettingsGyver.h>

extern GyverDBFile db;
extern SettingsGyver sett;

void sett_begin();
void sett_loop();
bool sett_started();

namespace cfg {
uint8_t bright();
bool power();
bool mlag();
bool link1();
bool link2();
bool nas();
uint8_t side_hue();
bool side_breath();
uint8_t side_speed();
String ssid();
String pass();
String ip();
bool close_ap();
}

DB_KEYS(
    kk,
    ssid,
    pass,
    close_ap,
    ip,
    power,
    bright,
    mlag,
    link1,
    link2,
    nas,
    side_hue,
    side_breath,
    side_speed
);
