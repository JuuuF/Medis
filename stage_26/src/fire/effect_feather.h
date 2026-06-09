#pragma once

#include "config.h"
#include "effects.h"
#include "communication.h"

#include <FastLED.h>

// ─────────────────────────────────────────────
// Parameters

#define FC_PULSE_WIDTH 5  // half-width in pixels

// Slave / strip parameters
#define FC_STRIP_MAX_DELAY_MS 500  // ms between each sequential strip activation
#define FC_STRIP_RISE_MS 800       // ms for a strip to fade in fully
#define FC_STRIP_HOLD_MS 600       // ms strip stays at peak
#define FC_STRIP_FALL_MS 700       // ms for strip to fade back out
#define FC_NUM_STRIPS 4

#define FC_FEATHER_HUE 32  // warm gold, matching body pulse
#define FC_FEATHER_SAT 200


class FeatherCascadeSlaveEffect : public Effect {
public:
  uint32_t effectStart;
  uint32_t stripDelays[FC_NUM_STRIPS];  // Dynamic delays per strip
  uint32_t totalMs;                     // Dynamic total duration

  FeatherCascadeSlaveEffect()
    : Effect(5) {
    effectStart = millis();

    uint32_t maxDelayFound = 0;
    for (uint8_t i = 0; i < FC_NUM_STRIPS; i++) {
      // Pick a random delay between 0 and your maximum setting
      stripDelays[i] = random16(FC_STRIP_MAX_DELAY_MS);
      if (stripDelays[i] > maxDelayFound) {
        maxDelayFound = stripDelays[i];
      }
    }
    // Total expected duration is when the slowest starting strip finishes its wave cycle
    totalMs = maxDelayFound + FC_STRIP_RISE_MS + FC_STRIP_HOLD_MS + FC_STRIP_FALL_MS;
  }

  void update() override {
    uint32_t now = millis();
    uint32_t elapsed = now - effectStart;

    if (elapsed >= totalMs) {
      done = true;
    }
  }
  void draw(CRGB *leds_1, CRGB *leds_2, CRGB *leds_3, CRGB *leds_4) override {
    uint32_t elapsed = millis() - effectStart;

    const uint32_t FC_PULSE_TRAVEL_MS = FC_STRIP_RISE_MS + FC_STRIP_HOLD_MS;

    for (uint8_t s = 0; s < FC_NUM_STRIPS; s++) {
      // Use the pre-calculated random start offset for this specific strip
      uint32_t stripStart = stripDelays[s];
      if (elapsed < stripStart) continue;

      uint32_t stripElapsed = elapsed - stripStart;

      if (stripElapsed >= (FC_PULSE_TRAVEL_MS + FC_STRIP_FALL_MS)) continue;

      int16_t waveCenter16 = 0;
      if (stripElapsed < FC_PULSE_TRAVEL_MS) {
        waveCenter16 = (stripElapsed * 39 * 256) / FC_PULSE_TRAVEL_MS;
      } else {
        waveCenter16 = 39 * 256;
      }
      uint8_t waveCenter = waveCenter16 >> 8;

      uint8_t globalAlpha = 255;
      if (stripElapsed > FC_PULSE_TRAVEL_MS) {
        uint32_t fallElapsed = stripElapsed - FC_PULSE_TRAVEL_MS;
        globalAlpha = lerp8by8(255, 0, (uint8_t)(fallElapsed * 255 / FC_STRIP_FALL_MS));
      }

      // --- CRITICAL FIX HERE ---
      // Select the correct target pointer array based on the current loop index 's'
      CRGB *targetStrip = nullptr;
      if (s == 0) targetStrip = leds_1;
      else if (s == 1) targetStrip = leds_2;
      else if (s == 2) targetStrip = leds_3;
      else if (s == 3) targetStrip = leds_4;

      if (targetStrip == nullptr) continue;

      for (uint8_t px = 0; px < 40; px++) {
        uint8_t dist = abs((int16_t)px - waveCenter);

        if (dist <= FC_PULSE_WIDTH) {
          uint8_t pulseFalloff = 255 - (dist * (255 / (FC_PULSE_WIDTH + 1)));

          uint8_t tipFalloff = lerp8by8(255, 160, (uint8_t)(px * 255 / 40));
          uint8_t finalBrightness = scale8(scale8(pulseFalloff, globalAlpha), tipFalloff);

          CHSV hsv(FC_FEATHER_HUE, FC_FEATHER_SAT, finalBrightness);
          CRGB col;
          hsv2rgb_rainbow(hsv, col);

          // We now write directly to indexes 0..39 of the active strip pointer safely
          targetStrip[px].r = qadd8(targetStrip[px].r, col.r);
          targetStrip[px].g = qadd8(targetStrip[px].g, col.g);
          targetStrip[px].b = qadd8(targetStrip[px].b, col.b);
        }
      }
    }
  }
};