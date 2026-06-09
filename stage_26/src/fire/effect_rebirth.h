#pragma once

#include "config.h"
#include "effects.h"
#include <FastLED.h>

// ----------------------------------------------
// Parameters (Optimized for faster, lightweight integer math)


#define REBIRTH_COLLAPSE_MS 15000  // ms: fire collapses inward
#define REBIRTH_DEAD_MS 3000       // ms: dead ember pulse, audience tension
#define REBIRTH_IGNITION_MS 150    // ms: white-hot ignition flash
#define REBIRTH_EXPANSION_MS 2000  // ms: explosive bloom expansion
#define REBIRTH_RECOVERY_MS 5000   // ms: fire returns stronger than before

#define REBIRTH_WING_WAVE_MS 600
#define REBIRTH_WING_SETTLE_MS 2000
#define REBIRTH_TAIL_DELAY_MS 250
#define REBIRTH_NUM_STRIPS 4
#define REBIRTH_STRIP_LEN 40

enum RebirthSlavePhase : uint8_t {
  RS_COLLAPSE = 0,
  RS_DEAD = 1,
  RS_WAVE = 2,
  RS_SETTLE = 3,
  RS_DONE = 4
};

class RebirthSlaveEffect : public Effect {
public:
  uint32_t phaseStart;
  RebirthSlavePhase phase;

  // Track just the front edge position per strip rather than a heavy 2D trail array
  uint8_t waveFront[REBIRTH_NUM_STRIPS];

  RebirthSlaveEffect()
    : Effect(4) {
    phaseStart = millis();
    phase = RS_COLLAPSE;
    memset(waveFront, 0, sizeof(waveFront));
  }

  void update() override {
    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    switch (phase) {
      case RS_COLLAPSE:
        if (elapsed >= REBIRTH_COLLAPSE_MS) {
          phase = RS_DEAD;
          phaseStart = now;
        }
        break;

      case RS_DEAD:
        if (elapsed >= REBIRTH_DEAD_MS) {  // Adjust the timing a little :)
          phase = RS_WAVE;
          phaseStart = now;
        }
        break;

      case RS_WAVE:
        // Main structural sweep phase
        if (elapsed >= (REBIRTH_WING_WAVE_MS + REBIRTH_TAIL_DELAY_MS)) {
          phase = RS_SETTLE;
          phaseStart = now;
        }
        break;

      case RS_SETTLE:
        if (elapsed >= REBIRTH_WING_SETTLE_MS) {
          phase = RS_DONE;
          done = true;
        }
        break;

      default:
        break;
    }
  }

  void draw(CRGB* leds_1, CRGB* leds_2, CRGB* leds_3, CRGB* leds_4) override {
    uint32_t elapsed = millis() - phaseStart;
    CRGB* stripPtrs[REBIRTH_NUM_STRIPS] = { leds_1, leds_2, leds_3, leds_4 };

    if (phase == RS_COLLAPSE) {
      uint8_t t = (uint8_t)((uint32_t)elapsed * 255 / REBIRTH_COLLAPSE_MS);
      uint8_t dimMul = lerp8by8(255, 0, t);
      uint8_t greenSup = lerp8by8(0, 160, t);
      for (uint8_t s = 0; s < REBIRTH_NUM_STRIPS; s++) {
        CRGB* strip = stripPtrs[s];
        for (uint8_t p = 0; p < REBIRTH_STRIP_LEN; p++) {
          strip[p].nscale8(dimMul);
          strip[p].g = qsub8(strip[p].g, (greenSup * strip[p].g) >> 8);
          strip[p].b = qsub8(strip[p].b, strip[p].b >> 1);
        }
      }
    } else if (phase == RS_DEAD) {
      for (uint8_t s = 0; s < REBIRTH_NUM_STRIPS; s++) {
        CRGB* strip = stripPtrs[s];
        for (uint8_t p = 0; p < REBIRTH_STRIP_LEN; p++) {
          strip[p] = CRGB::Black;
        }
      }
    }
    // --- PHASE 1: IGNITION WAVE SWEEP ---
    else if (phase == RS_WAVE) {
      for (uint8_t s = 0; s < REBIRTH_NUM_STRIPS; s++) {
        bool isTail = (s >= 2);
        uint32_t delay = isTail ? REBIRTH_TAIL_DELAY_MS : 0;

        if (elapsed < delay) {
          for (uint8_t p = 0; p < REBIRTH_STRIP_LEN; p++) {
            stripPtrs[s][p] = CRGB::Black;
          }
          continue;  // Waiting for tail lag to pass
        }
        uint32_t eff = elapsed - delay;

        // Calculate tip mapping directly using fast integer math
        uint16_t currentPos = (eff * REBIRTH_STRIP_LEN) / REBIRTH_WING_WAVE_MS;
        if (currentPos >= REBIRTH_STRIP_LEN) currentPos = REBIRTH_STRIP_LEN - 1;

        CRGB* strip = stripPtrs[s];

        for (uint8_t p = currentPos + 1; p < REBIRTH_STRIP_LEN; p++) {
          strip[p] = CRGB::Black;
        }

        strip[currentPos].r = qadd8(strip[currentPos].r, 255);
        strip[currentPos].g = qadd8(strip[currentPos].g, 240);
        strip[currentPos].b = qadd8(strip[currentPos].b, 150);  // White core

        if (currentPos > 0) {
          strip[currentPos - 1].r = qadd8(strip[currentPos - 1].r, 200);
          strip[currentPos - 1].g = qadd8(strip[currentPos - 1].g, 110);
        }
        if (currentPos > 1) {
          strip[currentPos - 2].r = qadd8(strip[currentPos - 2].r, 110);
          strip[currentPos - 2].g = qadd8(strip[currentPos - 2].g, 30);
        }
        // ADD THESE:
        if (currentPos > 2) {
          strip[currentPos - 3].r = qadd8(strip[currentPos - 3].r, 60);
          strip[currentPos - 3].g = qadd8(strip[currentPos - 3].g, 15);
        }
        if (currentPos > 3) {
          strip[currentPos - 4].r = qadd8(strip[currentPos - 4].r, 30);
        }
        if (currentPos > 4) {
          strip[currentPos - 5].r = qadd8(strip[currentPos - 5].r, 12);
        }
      }
    }
    // --- PHASE 1: SETTLE GLOW FADE ---
    else if (phase == RS_SETTLE) {
      // Linearly scale down a single byte value over time
      uint8_t t = (elapsed * 255) / REBIRTH_WING_SETTLE_MS;
      uint8_t glow = scale8(70, 255 - t);  // Dimming golden brush factor

      if (glow == 0) return;

      // Settle must fade out the whole strip, but we skip expensive multi-channel
      // multiplications per pixel by injecting cheap, fixed color ratios.
      for (uint8_t s = 0; s < REBIRTH_NUM_STRIPS; s++) {
        CRGB* strip = stripPtrs[s];
        for (uint8_t p = 0; p < REBIRTH_STRIP_LEN; p++) {
          strip[p].r = qadd8(strip[p].r, glow);
          strip[p].g = qadd8(strip[p].g, glow >> 1);  // Shifting right by 1 is faster than scale8
        }
      }
    }
  }
};