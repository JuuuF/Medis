#pragma once

#include "config.h"
#include "effects.h"
#include "communication.h"

#include <FastLED.h>

// ----------------------------------------------
// Parameters

#define EMBER_WAVE_TICKS_PER_STEP 1  // Lower = faster propagation
#define EMBER_TAIL_DELAY_MS 30       // Delay before tails react
#define EMBER_TAIL_DELAY_TICKS (EMBER_TAIL_DELAY_MS / 8)

// ----------------------------------------------
// Internal constants

#define EMBER_NUM_STRIPS 4

// ----------------------------------------------
class EmberEffectSlave : public Effect {
public:
  uint8_t frontPos[EMBER_NUM_STRIPS];  // current wave front (0–NUM_LEDS)
  uint8_t stepTick[EMBER_NUM_STRIPS];  // ticks since last front advance
  uint16_t globalTick;

  EmberEffectSlave()
    : Effect(3), globalTick(0) {
    Serial.println("Simple burst response start.");
    for (uint8_t s = 0; s < EMBER_NUM_STRIPS; s++) {
      frontPos[s] = 0;
      stepTick[s] = 0;
    }
  }

  void update() override {
    globalTick++;
    bool allDone = true;

    for (uint8_t s = 0; s < EMBER_NUM_STRIPS; s++) {
      uint16_t startTick = (s < 2) ? 0 : EMBER_TAIL_DELAY_TICKS;

      if (globalTick < startTick) {
        allDone = false;
        continue;
      }

      // Propagate the wave front straight to the end of the strip
      if (frontPos[s] < NUM_LEDS) {
        stepTick[s]++;
        if (stepTick[s] >= EMBER_WAVE_TICKS_PER_STEP) {
          stepTick[s] = 0;
          frontPos[s]++;
        }
        allDone = false;
      }
    }

    if (allDone) done = true;
  }

  void draw(CRGB *leds_1, CRGB *leds_2, CRGB *leds_3, CRGB *leds_4) override {
    CRGB *stripPtrs[EMBER_NUM_STRIPS] = { leds_1, leds_2, leds_3, leds_4 };

    for (uint8_t s = 0; s < EMBER_NUM_STRIPS; s++) {
      uint8_t front = frontPos[s];
      if (front == 0 || front >= NUM_LEDS) continue;

      CRGB *strip = stripPtrs[s];

      // 1. Traveling Leading Tip: Warm White (Balanced blue so it turns white over red fire)
      strip[front].r = qadd8(strip[front].r, 255);
      strip[front].g = qadd8(strip[front].g, 240);
      strip[front].b = qadd8(strip[front].b, 80);

      // 2. Short, clean tail structure (No loops over NUM_LEDS)
      if (front > 0) {
        // Pixel right behind the head: Bright Gold
        uint8_t t1 = front - 1;
        strip[t1].r = qadd8(strip[t1].r, 240);
        strip[t1].g = qadd8(strip[t1].g, 150);
        strip[t1].b = qadd8(strip[t1].b, 20);  // Subtle warm accent
      }
      if (front > 1) {
        // Second trailing pixel: Deep Amber
        uint8_t t2 = front - 2;
        strip[t2].r = qadd8(strip[t2].r, 180);
        strip[t2].g = qadd8(strip[t2].g, 70);
      }
      if (front > 2) {
        // Third trailing pixel: Cooling Red Ember
        uint8_t t3 = front - 3;
        strip[t3].r = qadd8(strip[t3].r, 90);
      }
    }
  }

  ~EmberEffectSlave() override {}
};