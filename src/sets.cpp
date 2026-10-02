#include "sets.h"

#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <WiFiConnector.h>

GyverDBFile db(&LittleFS, "/data.db");
SettingsGyver sett(PROJECT_NAME, &db);
static bool settings_started = false;

static String makeAPName() {
    return String(PROJECT_NAME);
}

// ========== build ==========
static void build(sets::Builder& b) {
    {
        sets::Group g(b, "Bare Metal");

        b.Switch(kk::power, "Питание");
        b.Slider(kk::bright, "Яркость", 0, 100, 1);
        
        b.Slider(kk::side_hue, "Цвет", 0, 255, 1);
        b.Switch(kk::side_breath, "Пульсация");
        b.Slider(kk::side_speed, "Скорость пульсации", 1, 30, 1);
    }

    {
        sets::Group g(b, "MLAG");
        b.Switch(kk::mlag, "MLAG (Active-Active)");
        b.Paragraph("Примечание", "Пока MLAG включен, Link 1 и Link 2 нельзя отключить отдельно");
        b.Switch(kk::link1, "Link 1");
        b.Switch(kk::link2, "Link 2");
    }

        b.Switch(kk::nas, "SDS");

    {
        sets::Group g(b, "Информация");
        b.Link("cloud.ru", "https://cloud.ru/");
    }

#if SHOW_WIFI_SETTINGS
    {
        sets::Group g(b, "WiFi");
        b.Input(kk::ssid, "SSID");
        b.Pass(kk::pass, "Password", "");
        b.Input(kk::ip, "IP", "0.0.0.0");
        if (b.Switch(kk::close_ap, "Close AP when STA connected")) {
            WiFiConnector.closeAP(db[kk::close_ap]);
        }
        if (b.Button("Connect")) {
            WiFiConnector.connect(db[kk::ssid], db[kk::pass]);
        }
    }
#endif

    if ((bool)db[kk::mlag]) {
        db[kk::link1] = true;
        db[kk::link2] = true;
    }
}

// ========== update ==========
static void update(sets::Updater& u) {
    (void)u;

    if ((bool)db[kk::mlag]) {
        if (!(bool)db[kk::link1]) db[kk::link1] = true;
        if (!(bool)db[kk::link2]) db[kk::link2] = true;
    }
}

// ========== begin ==========
void sett_begin() {
    if (settings_started) return;

#ifdef ESP32
    LittleFS.begin(true);
#else
    LittleFS.begin();
#endif

    db.begin();
    db.init(kk::ssid, "");
    db.init(kk::pass, "");
    db.init(kk::close_ap, true);
    db.init(kk::ip, "0.0.0.0");

    db.init(kk::power, true);
    db.init(kk::bright, 35);
    db.init(kk::mlag, true);
    db.init(kk::link1, true);
    db.init(kk::link2, true);
    db.init(kk::nas, true);
    db.init(kk::side_hue, 140);
    db.init(kk::side_breath, true);
    db.init(kk::side_speed, 9);

    WiFiConnector.onConnect([]() {
        const String ip = WiFi.localIP().toString();

        Serial.print("Connected: ");
        Serial.println(ip);

        if (String(db[kk::ip]) != ip) {
            db[kk::ip] = ip;
            db.update();
        }
    });

    WiFiConnector.onError([]() {
        Serial.print("Error. Start AP: ");
        Serial.println(WiFi.softAPIP());
    });

    WiFiConnector.setName(makeAPName());
    WiFiConnector.closeAP(db[kk::close_ap]);
    WiFiConnector.connect(db[kk::ssid], db[kk::pass]);

    sett.begin();
    sett.onBuild(build);
    sett.onUpdate(update);

    settings_started = true;
}

// ========== loop ==========
void sett_loop() {
    if (!settings_started) return;
    WiFiConnector.tick();
    sett.tick();
}

bool sett_started() {
    return settings_started;
}

namespace cfg {
uint8_t bright() {
    return (uint8_t)db[kk::bright];
}

bool power() {
    return (bool)db[kk::power];
}

bool mlag() {
    return (bool)db[kk::mlag];
}

bool link1() {
    return (bool)db[kk::link1];
}

bool link2() {
    return (bool)db[kk::link2];
}

bool nas() {
    return (bool)db[kk::nas];
}

uint8_t side_hue() {
    return (uint8_t)db[kk::side_hue];
}

bool side_breath() {
    return (bool)db[kk::side_breath];
}

uint8_t side_speed() {
    return (uint8_t)db[kk::side_speed];
}

String ssid() {
    return String(db[kk::ssid]);
}

String pass() {
    return String(db[kk::pass]);
}

String ip() {
    return String(db[kk::ip]);
}

bool close_ap() {
    return (bool)db[kk::close_ap];
}
}  // namespace cfg
