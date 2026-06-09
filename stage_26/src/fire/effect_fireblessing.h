#pragma once

#include "config.h"
#include "effects.h"
#include "communication.h"
#include <FastLED.h>

// ─────────────────────────────────────────────
// Timers (Mirrored exactly from Leader for unified state tracking)
#define FBS_DRAIN_MS 15000     // Ramp follower hue toward blue
#define FBS_HOLD_PALE_MS 2500  // Hold at peak blue shift
#define FBS_RESTORE_MS 7500    // Fade back to original fire

// Spatial Properties
#define FBS_STRIP_COUNT 4

// Luma gate: keeps dark backgrounds clean and un-tinted
#define FBS_LUMA_THRESH 20

// Target configurations matching the Leader's mystical blue aesthetic
#define FBS_TARGET_HUE 160
#define FBS_HUE_SHIFT_MAX 55
#define FBS_SAT_BOOST_MAX 60

enum FireBlessingSlavePhase : uint8_t {
  FBS_DRAINING = 0,
  FBS_HOLD_PALE,
  FBS_RESTORING,
  FBS_DONE
};

// ═════════════════════════════════════════════
// FOLLOWER EFFECT CLASS — Fire Blessing Blue-Shift
// ═════════════════════════════════════════════
class FireBlessingSlaveEffect : public Effect {
public:
  FireBlessingSlavePhase phase;
  uint32_t phaseStart;

  // 0-255 progress cursor matching the Leader's scaling value.
  // 255 = full blue shift applied, 0 = native follower colors.
  uint8_t shiftAmount;

  FireBlessingSlaveEffect()
    : Effect(10) { // Assuming ID 10 or your designated slave mapping ID
    Serial.println("Init FireBlessingSlaveEffect (blue-shift)");
    phaseStart = millis();
    phase = FBS_DRAINING;
    shiftAmount = 0;
  }

  // ─────────────────────────────────────────
  void update() override {
    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    switch (phase) {

      case FBS_DRAINING:
        // Linear ramp 0 → 255 over FBS_DRAIN_MS
        shiftAmount = (uint8_t)(elapsed * 255UL / FBS_DRAIN_MS);
        if (elapsed >= FBS_DRAIN_MS) {
          shiftAmount = 255;
          phase = FBS_HOLD_PALE;
          phaseStart = now;
        }
        break;

      case FBS_HOLD_PALE:
        // shiftAmount remains pinned at peak blue shift
        if (elapsed >= FBS_HOLD_PALE_MS) {
          phase = FBS_RESTORING;
          phaseStart = now;
        }
        break;

      case FBS_RESTORING:
        // Linear ramp 255 → 0 over FBS_RESTORE_MS
        shiftAmount = 255 - (uint8_t)(elapsed * 255UL / FBS_RESTORE_MS);
        if (elapsed >= FBS_RESTORE_MS) {
          shiftAmount = 0;
          phase = FBS_DONE;
          done = true;
        }
        break;

      default:
        break;
    }
  }

  // ─────────────────────────────────────────
  void draw(CRGB* leds_1, CRGB* leds_2, CRGB* leds_3, CRGB* leds_4) override {
    if (phase == FBS_DONE || shiftAmount == 0) return;

    CRGB* stripPtrs[FBS_STRIP_COUNT] = { leds_1, leds_2, leds_3, leds_4 };

    // Process all 4 physically separated follower strips sequentially
    for (uint8_t s = 0; s < FBS_STRIP_COUNT; s++) {
      CRGB* strip = stripPtrs[s];

      for (uint8_t p = 0; p < NUM_LEDS; p++) {
        CRGB& pix = strip[p];

        // ── Luma gate ──────────────────────────────────────────────────────
        // Keep background space completely un-mutated 
        uint8_t luma = scale8(pix.r, 77) + scale8(pix.g, 150) + scale8(pix.b, 29);
        if (luma < FBS_LUMA_THRESH) continue;

        // ── RGB → HSV ──────────────────────────────────────────────────────
        // Grab a fast, stack-allocated table approximation of the color space
        CHSV hsv = rgb2hsv_approximate(pix);

        // ── Hue rotation ───────────────────────────────────────────────────
        // Calculate the shortest path delta arc on the 256-unit color wheel
        int8_t rawDelta = (int8_t)((int16_t)FBS_TARGET_HUE - (int16_t)hsv.hue);
        
        // Restore sign boundaries after mathematical scale operations
        int16_t scaledDelta = (rawDelta >= 0)
                                ? (int16_t)scale8((uint8_t)rawDelta, shiftAmount)
                                : -(int16_t)scale8((uint8_t)(-rawDelta), shiftAmount);
        hsv.hue = (uint8_t)((int16_t)hsv.hue + scaledDelta);

        // ── Saturation boost ───────────────────────────────────────────────
        // Punch out the pale hot-cores of the slave flames into rich blues
        uint8_t satBoost = scale8(FBS_SAT_BOOST_MAX, shiftAmount);
        hsv.sat = qadd8(hsv.sat, satBoost);

        // Value (flickering structural brightness) remains unmodified

        // ── HSV → RGB ──────────────────────────────────────────────────────
        // Safely map the calculated structural data back directly into the array pointer
        hsv2rgb_rainbow(hsv, pix);
      }
    }
  }
};