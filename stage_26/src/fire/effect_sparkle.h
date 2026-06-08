#pragma once

#include "config.h"
#include "effects.h"
#include <FastLED.h>

// ----------------------------------------------

#define SPARKLE_DURATION_MS 2500
#define SPARKLE_DELAY_MS    1000
#define SPARKLE_COUNT 2  // Max sparkles per strip
#define SPARKLE_DECAY 40

// ----------------------------------------------


struct Sparkle {
  uint8_t x;
  uint8_t brightness;
  bool alive;
};


class SparkleEffect : public Effect {

private:
  Sparkle sparkles[4][SPARKLE_COUNT];  // 10 sparkles for each of the 4 strips
  uint32_t startTimes[4];              // Independent lifespans per strip
  uint8_t tickCounts[4] = { 0 };       // Independent spawn frequencies

public:
  SparkleEffect()
    : Effect(1) {
    uint32_t now = millis();
    for (uint8_t s = 0; s < 4; s++) {
      startTimes[s] = now;
      tickCounts[s] = random8(3);  // Offset spawn alignments slightly
      for (uint8_t i = 0; i < SPARKLE_COUNT; i++) {
        sparkles[s][i].alive = false;
      }
    }
  }

  void update() override {
    bool allStripsFinished = true;
    uint32_t now = millis();

    for (uint8_t s = 0; s < 4; s++) {
      uint32_t elapsed = now - startTimes[s];

      // The strip is active only AFTER the 1-second delay passes
      bool stripActive = (elapsed >= SPARKLE_DELAY_MS) && (elapsed <= (SPARKLE_DELAY_MS + SPARKLE_DURATION_MS));

      // Spawn sparkles independently for active strips
      if (stripActive && (++tickCounts[s] % 3 == 0)) {
        for (uint8_t i = 0; i < SPARKLE_COUNT; i++) {
          if (!sparkles[s][i].alive) {
            sparkles[s][i] = { (uint8_t)random8(NUM_LEDS), 150, true };
            break;
          }
        }
      }

      // Fade out sparkles independently (this remains unchanged)
      bool stripHasSparks = false;
      for (uint8_t i = 0; i < SPARKLE_COUNT; i++) {
        if (!sparkles[s][i].alive) continue;

        stripHasSparks = true;
        if (sparkles[s][i].brightness < SPARKLE_DECAY) {
          sparkles[s][i].alive = false;
        } else {
          sparkles[s][i].brightness -= SPARKLE_DECAY;
        }
      }

      // If we haven't reached the end window yet, or sparkles are still fading, stay alive
      if ((elapsed <= (SPARKLE_DELAY_MS + SPARKLE_DURATION_MS)) || stripHasSparks) {
        allStripsFinished = false;
      }
    }

    if (allStripsFinished) {
      done = true;
    }
  }

  void draw(CRGB *leds_1, CRGB *leds_2, CRGB *leds_3, CRGB *leds_4) override {
    // Array of array pointers makes drawing a simple loop
    CRGB *stripPointers[4] = { leds_1, leds_2, leds_3, leds_4 };

    for (uint8_t s = 0; s < 4; s++) {
      CRGB *currentStrip = stripPointers[s];

      for (uint8_t i = 0; i < SPARKLE_COUNT; i++) {
        if (!sparkles[s][i].alive) continue;

        uint8_t idx = sparkles[s][i].x;
        uint8_t bright = sparkles[s][i].brightness;

        currentStrip[idx] += CRGB(bright, bright, bright);
      }
    }
  }
};
