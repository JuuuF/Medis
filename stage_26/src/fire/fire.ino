#include <FastLED.h>

#include "config.h"
#include "fire.h"
#include "communication.h"

// -----------------------------------------------------------------------

CRGB leds_1[NUM_LEDS];
CRGB leds_2[NUM_LEDS];
CRGB leds_3[NUM_LEDS];
CRGB leds_4[NUM_LEDS];

// -----------------------------------------------------------------------

void setup() {
  pinMode(LED_PIN_1, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);
  pinMode(LED_PIN_3, OUTPUT);
  pinMode(LED_PIN_4, OUTPUT);
  FastLED.addLeds<CHIPSET, LED_PIN_1, COLOR_ORDER>(leds_1, NUM_LEDS);
  FastLED.addLeds<CHIPSET, LED_PIN_2, COLOR_ORDER>(leds_2, NUM_LEDS);
  FastLED.addLeds<CHIPSET, LED_PIN_3, COLOR_ORDER>(leds_3, NUM_LEDS);
  FastLED.addLeds<CHIPSET, LED_PIN_4, COLOR_ORDER>(leds_4, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setDither(DISABLE_DITHER);
  fill_solid(leds_1, NUM_LEDS, CRGB::Black);
  fill_solid(leds_2, NUM_LEDS, CRGB::Black);
  fill_solid(leds_3, NUM_LEDS, CRGB::Black);
  fill_solid(leds_4, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // Communication setup
  pinMode(RX_PIN, INPUT);

  Serial.begin(9600);
}

void loop() {

  EVERY_N_MILLISECONDS(8) {

    update_fire();

    Fire1D(leds_1, 0);
    Fire1D(leds_2, 1);
    Fire1D(leds_3, 2);
    Fire1D(leds_4, 3);
    FastLED.show();
  }

  if (digitalRead(RX_PIN) == HIGH) {
    uint8_t effectID = receiveEffect();
    Serial.print("Received effect: ");
    Serial.println(effectID);
  }
}

// -----------------------------------------------------------------------
