#pragma once

#include "config.h"
#include "effects.h"
#include "communication.h"
#include <FastLED.h>

// ─────────────────────────────────────────────
// Hardware Spatial Configuration
#define FWS_STRIP_COUNT 4

// ─────────────────────────────────────────────
// Phase Durations
#define FWS_TEAR_MS 4000
#define FWS_TAIL_DELAY_MS 400
#define FWS_BLOOM_MS 5300

// ── TWEAK THIS HANDLE TO CHANGE SPEED FOR ALL STRIPS ─────────────────────
// Units: 64ths of a pixel per 8ms frame.
// Higher values = Faster travel. Lower values = Slower travel.
// 64 = 1 full pixel per frame. 32 = 0.5 pixels per frame.
#define FWS_RAW_SPEED 15

// Bloom Peak Intensities
#define FWS_BLOOM_PEAK_R 230
#define FWS_BLOOM_PEAK_G 105

enum FWSPhase : uint8_t {
  FWS_STREAKING = 0,
  FWS_BLOOM,
  FWS_DONE
};

// ═════════════════════════════════════════════
// SLAVE EFFECT CLASS  —  Effect ID 8
// ═════════════════════════════════════════════
class FawkesWeepsSlaveEffect : public Effect {
public:
  FWSPhase phase;
  uint32_t phaseStart;

  uint32_t posFP[FWS_STRIP_COUNT];
  bool streakActive[FWS_STRIP_COUNT];
  bool streakDone[FWS_STRIP_COUNT];

  FawkesWeepsSlaveEffect()
    : Effect(8) {
    phaseStart = millis();
    phase = FWS_STREAKING;

    for (uint8_t i = 0; i < FWS_STRIP_COUNT; i++) {
      posFP[i] = 0;
      streakActive[i] = (i < 2);  // Wings live immediately; Tails wait for stagger
      streakDone[i] = false;
    }
  }

  // ─────────────────────────────────────────────
  void update() override {
    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    if (phase == FWS_STREAKING) {

      // Activate tail strips after stagger lag window passes
      if (elapsed >= FWS_TAIL_DELAY_MS) {
        streakActive[2] = true;
        streakActive[3] = true;
      }

      uint32_t maxFP = (uint32_t)(NUM_LEDS - 1) * 64UL;

      for (uint8_t i = 0; i < FWS_STRIP_COUNT; i++) {
        if (!streakActive[i] || streakDone[i]) continue;

        // Unified manual speed applied across all lines
        posFP[i] += FWS_RAW_SPEED;

        if (posFP[i] >= maxFP) {
          posFP[i] = maxFP;
          streakDone[i] = true;
        }
      }

      if (elapsed >= FWS_TEAR_MS) {
        phase = FWS_BLOOM;
        phaseStart = now;
      }
    } else if (phase == FWS_BLOOM) {
      if (elapsed >= FWS_BLOOM_MS) {
        phase = FWS_DONE;
        done = true;
      }
    }
  }

  // ─────────────────────────────────────────────
  void draw(CRGB* leds_1, CRGB* leds_2, CRGB* leds_3, CRGB* leds_4) override {
    CRGB* stripPtrs[FWS_STRIP_COUNT] = { leds_1, leds_2, leds_3, leds_4 };

    if (phase == FWS_STREAKING) {
      for (uint8_t s = 0; s < FWS_STRIP_COUNT; s++) {
        if (!streakActive[s]) continue;

        CRGB* strip = stripPtrs[s];
        uint8_t fallen = (uint8_t)(posFP[s] >> 6);
        int8_t head = (int8_t)(NUM_LEDS - 1) - (int8_t)fallen;
        uint8_t frac = (uint8_t)((posFP[s] & 0x3F) << 2);

        if (streakDone[s]) {
          head = 0;
          frac = 0;
        }

        // ── Core & Anti-Aliased Wave Front ──
        _paintStreak(strip, head, 160, 200, 255, 255 - frac);
        _paintStreak(strip, head - 1, 160, 200, 255, frac);

        // ── Thermal Graduated Decay Trail ──
        _paintStreak(strip, head + 1, 100, 130, 255, 220);
        _paintStreak(strip, head + 2, 55, 75, 200, 170);
        _paintStreak(strip, head + 3, 25, 35, 150, 110);
        _paintStreak(strip, head + 4, 12, 18, 100, 65);
        _paintStreak(strip, head + 5, 5, 8, 50, 38);
      }
    }
    // ════════════════════════════════════════
    // GOLDEN BLOOM
    // ════════════════════════════════════════
    else if (phase == FWS_BLOOM) {
      uint32_t elapsed = millis() - phaseStart;
      uint8_t factor = (uint8_t)((elapsed * 255UL) / FWS_BLOOM_MS);
      uint8_t glowR = scale8(FWS_BLOOM_PEAK_R, 255 - factor);
      uint8_t glowG = scale8(FWS_BLOOM_PEAK_G, 255 - factor);

      if (glowR == 0) return;

      for (uint8_t s = 0; s < FWS_STRIP_COUNT; s++) {
        CRGB* strip = stripPtrs[s];
        for (uint8_t p = 0; p < NUM_LEDS; p++) {
          uint8_t rootFrac = (uint8_t)(255UL * (NUM_LEDS - 1 - p) / (NUM_LEDS - 1));
          uint8_t localR = scale8(glowR, lerp8by8(80, 255, rootFrac));
          uint8_t localG = scale8(glowG, lerp8by8(40, 200, rootFrac));

          strip[p].r = qadd8(strip[p].r, localR);
          strip[p].g = qadd8(strip[p].g, localG);
        }
      }
    }
  }

private:
  inline void _paintStreak(CRGB* strip, int8_t pos,
                           uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
    if (pos < 0 || pos >= (int8_t)NUM_LEDS) return;
    strip[pos].r = qadd8(strip[pos].r, scale8(r, alpha));
    strip[pos].g = qadd8(strip[pos].g, scale8(g, alpha));
    strip[pos].b = qadd8(strip[pos].b, scale8(b, alpha));
  }
};