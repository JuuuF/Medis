#pragma once

#include "config.h"
#include "effects.h"
#include "communication.h"
#include <FastLED.h>

// ─────────────────────────────────────────────
// Hardware Spatial Configurations
#define SBS_STRIP_COUNT 4
#define SBS_STRIP_LEN 40  // Hard physical boundary per channel

// Pulse travel timing: single pass root → tip
// Calibrated so the wave clears index 39 in ~700ms
// Fixed-point velocity: pixels per 8ms tick in 256ths-of-a-pixel
// Target: 40 pixels / (700ms / 8ms) = 40/87 ≈ 0.46 px/tick → FP256 = ~118
#define SBS_PULSE_SPEED_FP 118  // 256-scaled fixed-point pixels per 8ms tick

// Trail length behind the hot head (pixels rendered in decay gradient)
#define SBS_TRAIL_LEN 6

// Hold phase dims down strips to emphasize inhalation
// During SBS_IN_HOLD a gentle suppression is applied so strips look drained
#define SBS_HOLD_SCALE 40  // 40/255 ≈ 15% — strips appear almost extinguished

enum SBSPhase : uint8_t {
  SBS_IN_HOLD = 0,  // Synchronized with SB_COMPRESS + SB_HEAT on leader
  SBS_EX_PULSE,     // Single thermal sweep root → tip on all 4 strips
  SBS_DONE
};

// ═════════════════════════════════════════════
// FOLLOWER EFFECT CLASS — Sacred Breath
// ═════════════════════════════════════════════
class SacredBreathSlaveEffect : public Effect {
public:
  SBSPhase phase;
  uint32_t phaseStart;

  // Per-strip fixed-point position tracker (256-scaled, unit = 1/256th pixel)
  // Declared as uint16_t to safely represent 0..(SBS_STRIP_LEN-1)*256
  uint16_t posFP[SBS_STRIP_COUNT];

  // Tracks whether each strip's wave has fully exited index 39+trail
  bool stripDone[SBS_STRIP_COUNT];

  SacredBreathSlaveEffect()
    : Effect(9) {
    phaseStart = millis();
    phase = SBS_IN_HOLD;

    for (uint8_t s = 0; s < SBS_STRIP_COUNT; s++) {
      posFP[s] = 0;
      stripDone[s] = false;
    }
  }

  // ─────────────────────────────────────────────
  void update() override {
    if (phase == SBS_IN_HOLD) {
      // This phase waits for the Leader's SB_RELEASE broadcast trigger.
      // In practice the runtime spawns this class exactly when the Leader
      // enters SB_RELEASE, so we transition immediately on the first tick.
      // If your architecture passes a deferred start signal instead, replace
      // this block with the appropriate network-flag check.
      phase = SBS_EX_PULSE;
      phaseStart = millis();
      return;
    }

    if (phase == SBS_EX_PULSE) {
      // Advance each strip's fixed-point position by the speed constant
      uint8_t allDone = 0;
      for (uint8_t s = 0; s < SBS_STRIP_COUNT; s++) {
        if (stripDone[s]) {
          allDone++;
          continue;
        }

        // Advance head position
        posFP[s] += SBS_PULSE_SPEED_FP;

        // Head has fully cleared the strip (including trail overshoot)
        // Done threshold: head index > SBS_STRIP_LEN - 1 + SBS_TRAIL_LEN
        uint16_t headIdx = posFP[s] >> 8;  // integer pixel
        if (headIdx >= (uint16_t)(SBS_STRIP_LEN + SBS_TRAIL_LEN)) {
          stripDone[s] = true;
          allDone++;
        }
      }

      if (allDone == SBS_STRIP_COUNT) {
        phase = SBS_DONE;
        done = true;
      }
    }
  }

  // ─────────────────────────────────────────────
  void draw(CRGB* leds_1, CRGB* leds_2, CRGB* leds_3, CRGB* leds_4) override {
    CRGB* stripPtrs[SBS_STRIP_COUNT] = { leds_1, leds_2, leds_3, leds_4 };

    // ── Hold Phase: dim all strips to simulate energy pulled into core ──────
    if (phase == SBS_IN_HOLD) {
      for (uint8_t s = 0; s < SBS_STRIP_COUNT; s++) {
        CRGB* strip = stripPtrs[s];
        for (uint8_t p = 0; p < SBS_STRIP_LEN; p++) {
          // Non-destructive dimming: nscale8_video preserves hue, just dims
          strip[p].nscale8_video(SBS_HOLD_SCALE);
        }
      }
      return;
    }

    // ── Pulse Phase: anti-aliased thermal streak root → tip ─────────────────
    if (phase == SBS_EX_PULSE) {
      for (uint8_t s = 0; s < SBS_STRIP_COUNT; s++) {
        if (stripDone[s]) continue;

        CRGB* strip = stripPtrs[s];
        uint16_t headFP = posFP[s];
        uint16_t headIdx = headFP >> 8;           // integer pixel index
        uint8_t frac = (uint8_t)(headFP & 0xFF);  // sub-pixel fraction (0-255)

        // ── HEAD pixel: white-hot core ──────────────────────────────────────
        // Anti-alias: blend between headIdx and headIdx+1 using frac
        if (headIdx < SBS_STRIP_LEN) {
          // Leading edge pixel brightness scaled by (1 - frac) → full at frac=0
          uint8_t leadAlpha = 255 - frac;
          strip[headIdx].r = qadd8(strip[headIdx].r, scale8(255, leadAlpha));
          strip[headIdx].g = qadd8(strip[headIdx].g, scale8(240, leadAlpha));
          strip[headIdx].b = qadd8(strip[headIdx].b, scale8(160, leadAlpha));
        }

        // Sub-pixel bleed one pixel ahead of the integer position
        if (frac > 0 && (headIdx + 1) < SBS_STRIP_LEN) {
          strip[headIdx + 1].r = qadd8(strip[headIdx + 1].r, scale8(255, frac));
          strip[headIdx + 1].g = qadd8(strip[headIdx + 1].g, scale8(240, frac));
          strip[headIdx + 1].b = qadd8(strip[headIdx + 1].b, scale8(160, frac));
        }

        // ── TRAILING HEAT GRADIENT ──────────────────────────────────────────
        // 6 pixels behind the head, cooling from bright gold → deep amber → faint red
        // Trail color table: index 0 = immediately behind head
        static const uint8_t trailR[SBS_TRAIL_LEN] = { 255, 220, 180, 120, 70, 30 };
        static const uint8_t trailG[SBS_TRAIL_LEN] = { 200, 140, 90, 50, 20, 8 };
        static const uint8_t trailB[SBS_TRAIL_LEN] = { 60, 20, 8, 4, 2, 0 };

        for (uint8_t t = 0; t < SBS_TRAIL_LEN; t++) {
          // Trail pixel is behind (lower index) the head
          // Safe underflow check: headIdx must be > t for this pixel to exist
          if (headIdx < (uint16_t)(t + 1)) break;
          uint8_t trailPx = (uint8_t)(headIdx - 1 - t);
          if (trailPx >= SBS_STRIP_LEN) break;

          strip[trailPx].r = qadd8(strip[trailPx].r, trailR[t]);
          strip[trailPx].g = qadd8(strip[trailPx].g, trailG[t]);
          strip[trailPx].b = qadd8(strip[trailPx].b, trailB[t]);
        }
      }
      return;
    }
  }
};
