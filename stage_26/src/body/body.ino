#include <FastLED.h>

// -----------------------------------------------------------------------
// Strip settings

#include "config.h"
#include "fire.h"
#include "effects.h"
#include "communication.h"

CRGB leds[NUM_LEDS];
Effect *activeEffect = nullptr;

// -----------------------------------------------------------------------

void setup() {
  pinMode(LED_PIN, OUTPUT);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setDither(DISABLE_DITHER);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // Communication Setup
  pinMode(TX_PIN, OUTPUT);
  digitalWrite(TX_PIN, LOW);

  Serial.begin(9600);
}

void loop() {
  static uint8_t i = 0;

  EVERY_N_MILLISECONDS(8) {

    // Fire handling
    update_fire_values();

    for (uint8_t col = 0; col < MATRIX_WIDTH; col++) {
      FireBody(col);
      FireHead(col);
    }
    // fill_solid(leds, NUM_LEDS, CRGB::Black);

    // Effect handling
    if (activeEffect != nullptr) {
      activeEffect->update();
      if (activeEffect->done) {
        delete activeEffect;
        activeEffect = nullptr;
      } else {
        activeEffect->draw(leds);
      }
    }

    maybeSpawnEffect();

    FastLED.show();
  }

  // if (random() % 100 == 0) {
  //   broadcastEffect(i);
  //   i++;
  // }
}
