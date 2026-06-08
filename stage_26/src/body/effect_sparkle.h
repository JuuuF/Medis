#pragma once

#include "config.h"
#include "effects.h"
#include "mapping.h"

// ----------------------------------------------

#define SPARKLE_DURATION_MS 2500
#define SPARKLE_COUNT 10  // max simultaneous sparkles
#define SPARKLE_DECAY 40  // how fast each sparkle fades: 0=slow, 255=fast

// ----------------------------------------------

struct Sparkle {
  uint8_t x, y;
  uint8_t brightness;
  bool alive;
};


class SparkleEffect : public Effect {
  Sparkle sparkles[SPARKLE_COUNT];
  uint32_t startTime;
  uint8_t tickCount = 0;

public:
  SparkleEffect() {
    startTime = millis();
    for (uint8_t i = 0; i < SPARKLE_COUNT; i++) {
      sparkles[i].alive = false;
    }
  }

  void update() override {

    // When time is up, start fading the effect
    if (millis() - startTime > SPARKLE_DURATION_MS) {
      active = false;
    }

    if (!active) {
      // When ending the effect, check if all sparks are gone
      bool anyAlive = false;
      for (int i = 0; i < SPARKLE_COUNT; i++) {
        if (sparkles[i].alive) {
          anyAlive = true;
          break;
        }
      }
      if (!anyAlive) {
        done = true;
        return;
      }
    } else if (++tickCount % 3 == 0) {
      // Every few ticks, spawn a new sparkle in a dead slot
      for (uint8_t i = 0; i < SPARKLE_COUNT; i++) {
        if (!sparkles[i].alive) {
          sparkles[i] = {
            (uint8_t)random8(MATRIX_WIDTH),
            (uint8_t)random8(MATRIX_HEIGHT),
            255,
            true
          };
          break;
        }
      }
    }

    // Decay all live sparkles
    for (uint8_t i = 0; i < SPARKLE_COUNT; i++) {
      if (!sparkles[i].alive) continue;
      if (sparkles[i].brightness < SPARKLE_DECAY) {
        sparkles[i].alive = false;
      } else {
        sparkles[i].brightness -= SPARKLE_DECAY;
      }
    }
  }

  void draw(CRGB *leds) override {
    for (uint8_t i = 0; i < SPARKLE_COUNT; i++) {
      if (!sparkles[i].alive) continue;
      uint16_t idx = XY(sparkles[i].x, sparkles[i].y);
      // Additive white blend so sparkle sits on top of the flame
      leds[idx] += CRGB(sparkles[i].brightness,
                        sparkles[i].brightness,
                        sparkles[i].brightness);
    }
  }
};
