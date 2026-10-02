#include <Arduino.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>

#include "sets.h"

#define LED_PIN 5
#define LED_AMOUNT 6

static const uint8_t LED_SIDE = 0;
static const uint8_t LED_SW_1 = 1;
static const uint8_t LED_SW_2 = 2;
static const uint8_t LED_SRV_1 = 3;
static const uint8_t LED_SRV_2 = 4;
static const uint8_t LED_NAS = 5;

CRGB leds[LED_AMOUNT];

ESP8266WebServer otaServer(8080);
ESP8266HTTPUpdateServer otaUpdater;

struct TrafficBlink {
  uint32_t nextTick = 0;
  uint8_t value = 0;
  uint8_t mix = 0;
  bool on = false;
};

TrafficBlink traffic[LED_AMOUNT];
uint8_t sidePhase = 0;

static void setupOtaWeb() {
  otaUpdater.setup(&otaServer, "/update");
  otaServer.begin();

  Serial.println("OTA web enabled at /update on port 8080");
  Serial.print("STA URL: http://");
  Serial.print(WiFi.localIP());
  Serial.println(":8080/update");
  Serial.print("AP  URL: http://");
  Serial.print(WiFi.softAPIP());
  Serial.println(":8080/update");
}

static CRGB makeTrafficColor(uint8_t value, uint8_t mix) {
  CRGB yellowish = CHSV(40, 210, value);
  CRGB greenish = CHSV(95, 190, value);
  return blend(yellowish, greenish, mix);
}

static void renderTrafficLed(uint8_t ledIndex, bool enabled) {
  TrafficBlink& t = traffic[ledIndex];

  if (!enabled) {
    t.on = false;
    leds[ledIndex] = CRGB::Black;
    return;
  }

  const uint32_t now = millis();
  if (now >= t.nextTick) {
    t.on = random8() < 200;
    t.value = random8(120, 255);
    t.mix = random8(30, 220);
    t.nextTick = now + random16(70, 260);
  }

  if (!t.on) {
    leds[ledIndex] = CRGB(10, 14, 0);
    return;
  }

  leds[ledIndex] = makeTrafficColor(t.value, t.mix);
}

static void renderRack() {
  static uint32_t frameTmr = 0;
  static uint32_t lastAnimMs = 0;
  static uint16_t sideAccum = 0;

  const uint32_t now = millis();
  if (now - frameTmr < 30) return;
  frameTmr = now;

  uint32_t dt = lastAnimMs ? (now - lastAnimMs) : 30;
  lastAnimMs = now;
  if (dt > 300) dt = 300;

  if (!cfg::power()) {
    fill_solid(leds, LED_AMOUNT, CRGB::Black);
    FastLED.show();
    return;
  }

  FastLED.setBrightness(map(cfg::bright(), 0, 100, 0, 255));

  sideAccum += (uint16_t)cfg::side_speed() * (uint16_t)dt;
  sidePhase += sideAccum / 30;
  sideAccum %= 30;

  if (cfg::side_breath()) {
    const uint8_t sideValue = qadd8(25, scale8(sin8(sidePhase), 230));
    leds[LED_SIDE] = CHSV(cfg::side_hue(), 255, sideValue);
  } else {
    leds[LED_SIDE] = CRGB::Black;
  }

  const bool mlag = cfg::mlag();
  const bool link1 = mlag || cfg::link1();
  const bool link2 = mlag || cfg::link2();

  renderTrafficLed(LED_SW_1, link1);
  renderTrafficLed(LED_SW_2, link2);
  renderTrafficLed(LED_SRV_1, link1);
  renderTrafficLed(LED_SRV_2, link2);
  renderTrafficLed(LED_NAS, cfg::nas());

  FastLED.show();
}

void setup() {
  delay(200);
  Serial.begin(115200);
  Serial.println("\nBareMetal Rack boot");
  randomSeed(micros());

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_AMOUNT).setCorrection(TypicalLEDStrip);
  FastLED.setDither(0);
  FastLED.clear();
  FastLED.show();

  sett_begin();
  setupOtaWeb();
}

void loop() {
  static uint32_t settTickTmr = 0;
  static uint32_t otaTickTmr = 0;
  static bool netPhase = false;

  renderRack();

  const uint32_t now = millis();

  // Разносим сетевые обработчики по времени, чтобы они реже забирали
  // подряд большие куски CPU и не дёргали анимацию.
  if (netPhase) {
    if (now - settTickTmr >= 2) {
      settTickTmr = now;
      sett_loop();
    }
  } else {
    if (now - otaTickTmr >= 2) {
      otaTickTmr = now;
      otaServer.handleClient();
    }
  }
  netPhase = !netPhase;

  yield();
}
