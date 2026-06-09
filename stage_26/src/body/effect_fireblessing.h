#pragma once

#include "config.h"
#include "effects.h"
#include "mapping.h"
#include "communication.h"
#include <FastLED.h>

// ─────────────────────────────────────────────
// Timers
#define FB_DRAIN_MS 15000     // Ramp fire hue toward blue
#define FB_HOLD_PALE_MS 2500  // Hold at peak blue shift
#define FB_RESTORE_MS 7500    // Fade back to original fire

// Luma gate: don't touch near-black pixels (avoids background flicker)
#define FB_LUMA_THRESH 20

// Blue-shift target on the FastLED hue wheel (160 = cool blue)
#define FB_TARGET_HUE 160

// Maximum hue rotation distance applied at peak (0-128 wheel units).
// 55 gives a vivid, clearly mystical blue without washing out to cyan.
#define FB_HUE_SHIFT_MAX 55

// Saturation boost applied at peak (pushes muted flame reds toward vivid blue).
// Expressed as an additive delta (0-255 scale8 space).
#define FB_SAT_BOOST_MAX 60

enum PalePhase : uint8_t {
  FB_DRAINING = 0,
  FB_HOLD_PALE,
  FB_RESTORING,
  FB_DONE
};

// ═════════════════════════════════════════════
class FireBlessingEffect : public Effect {
public:
  PalePhase phase;
  uint32_t phaseStart;

  // 0-255 progress cursor shared by DRAINING and RESTORING.
  // 255 = full blue shift applied, 0 = native fire colours.
  uint8_t shiftAmount;

  FireBlessingEffect(uint8_t id)
    : Effect(id) {
    Serial.println("Init FireBlessingEffect (blue-shift)");
    phaseStart = millis();
    phase = FB_DRAINING;
    shiftAmount = 0;
    broadcastEffect(effectID);
  }

  // ─────────────────────────────────────────
  void update() override {
    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    switch (phase) {

      case FB_DRAINING:
        // Linear ramp 0 → 255 over FB_DRAIN_MS
        shiftAmount = (uint8_t)(elapsed * 255UL / FB_DRAIN_MS);
        if (elapsed >= FB_DRAIN_MS) {
          shiftAmount = 255;
          phase = FB_HOLD_PALE;
          phaseStart = now;
        }
        break;

      case FB_HOLD_PALE:
        // shiftAmount stays pinned at 255
        if (elapsed >= FB_HOLD_PALE_MS) {
          phase = FB_RESTORING;
          phaseStart = now;
        }
        break;

      case FB_RESTORING:
        // Linear ramp 255 → 0 over FB_RESTORE_MS
        shiftAmount = 255 - (uint8_t)(elapsed * 255UL / FB_RESTORE_MS);
        if (elapsed >= FB_RESTORE_MS) {
          shiftAmount = 0;
          phase = FB_DONE;
          done = true;
        }
        break;

      default:
        break;
    }
  }

  // ─────────────────────────────────────────
  void draw(CRGB* leds) override {
    if (phase == FB_DONE || shiftAmount == 0) return;

    for (uint16_t idx = 0; idx < NUM_LEDS; idx++) {
      CRGB& pix = leds[idx];

      // ── Luma gate ──────────────────────────────────────────────────────
      // Skip near-black pixels entirely so the dark background never
      // gets contaminated with a faint blue tint.
      uint8_t luma = scale8(pix.r, 77) + scale8(pix.g, 150) + scale8(pix.b, 29);
      if (luma < FB_LUMA_THRESH) continue;

      // ── RGB → HSV ──────────────────────────────────────────────────────
      // FastLED's rgb2hsv_approximate gives us a mutable HSV triple
      // without any sqrt or division — pure table lookup internally.
      CHSV hsv = rgb2hsv_approximate(pix);

      // ── Hue rotation ───────────────────────────────────────────────────
      // Compute how far we need to rotate from the current hue to the
      // target blue hue, then scale that rotation by shiftAmount.
      //
      // We work in signed 8-bit delta space (–128 to +127) so the
      // rotation always picks the *shorter* arc around the 256-unit wheel.
      int8_t rawDelta = (int8_t)((int16_t)FB_TARGET_HUE - (int16_t)hsv.hue);
      // scale8 operates on magnitude; sign is restored after.
      int16_t scaledDelta = (rawDelta >= 0)
                              ? (int16_t)scale8((uint8_t)rawDelta, shiftAmount)
                              : -(int16_t)scale8((uint8_t)(-rawDelta), shiftAmount);
      hsv.hue = (uint8_t)((int16_t)hsv.hue + scaledDelta);

      // ── Saturation boost ───────────────────────────────────────────────
      // Flame pixels can be low-saturation (near-white hot cores).
      // Boosting saturation ensures the hue shift is actually *visible*
      // instead of just nudging a near-grey pixel slightly bluer.
      uint8_t satBoost = scale8(FB_SAT_BOOST_MAX, shiftAmount);
      hsv.sat = qadd8(hsv.sat, satBoost);

      // Value (brightness) is left completely untouched — the fire's
      // natural intensity structure is preserved 1:1.

      // ── HSV → RGB and write back ───────────────────────────────────────
      hsv2rgb_rainbow(hsv, pix);
    }
  }
};