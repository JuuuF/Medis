#pragma once

#include "config.h"
#include "effects.h"
#include "communication.h"
#include <FastLED.h>

// ─────────────────────────────────────────────
// Hardware Spatial Configurations & Structural Rules
#define CA_SLAVE_STRIP_COUNT 4

// ── Wing ripple constants ────────────────────────────────────────────────────
// Wings execute CA_WING_RIPPLE_COUNT rapid feather-ripple passes.
// Each ripple's velocity is scaled upward each pass by CA_WING_ACCEL_SHIFT bits.
#define CA_WING_RIPPLE_COUNT 5     // Total aerodynamic ripple passes per wing
#define CA_WING_BASE_SPEED_FP 180  // Fixed-point speed for ripple 0 (pixels per second * 64)
#define CA_WING_ACCEL_SHIFT 1      // velocity <<= 1 per completed ripple (doubles each pass)

// ── Tail streamer constants ──────────────────────────────────────────────────
// Tails use two independent layered velocities; strip 2 slightly faster.
#define CA_TAIL_DELAY_MS 220   // Tail activation lag after wing launch
#define CA_TAIL2_SPEED_FP 260  // Fixed-point speed strip 2 (faster)
#define CA_TAIL3_SPEED_FP 190  // Fixed-point speed strip 3 (slower, trailing)

// ── Shared rendering palette constants ──────────────────────────────────────
// Head pixel: white-hot (255,255,200)
// -1 from head: bright orange (255,130,0)
// -2: amber (255,70,0)
// -3: deep orange (200,30,0)
// -4: charcoal red (100,5,0)
// beyond: zero additive contribution

// ── Settle/cool-down phase ───────────────────────────────────────────────────
#define CA_SLAVE_SETTLE_MS 900  // Duration of the post-wave cool-down glow

// ─────────────────────────────────────────────
// Phase Definitions
enum CASlavePhase : uint8_t {
  CAS_PROPAGATING = 0,
  CAS_SETTLE,
  CAS_DONE
};

// ═════════════════════════════════════════════
// SLAVE EFFECT CLASS
// ═════════════════════════════════════════════
class CelestialAscensionSlaveEffect : public Effect {
public:
  uint32_t phaseStart;
  CASlavePhase phase;

  // ── Wing strip state (strips 0 & 1) ──────────────────────────────────────
  // Each wing tracks: which ripple pass it is on, current fixed-point position,
  // and a snapshot of the current pass's velocity.
  uint8_t wingRipple[2];    // Current ripple index per wing (0 … CA_WING_RIPPLE_COUNT-1)
  uint16_t wingPosFP[2];    // Fixed-point position within current ripple (0…strip_len*64)
  uint16_t wingSpeedFP[2];  // Current ripple speed (FP) per wing
  bool wingDone[2];         // Wing has completed all ripple passes

  // ── Tail strip state (strips 2 & 3) ──────────────────────────────────────
  uint16_t tailPosFP[2];  // Fixed-point tail position (strips 2,3)
  bool tailActive[2];     // Tail has passed its delay window
  bool tailDone[2];       // Tail has fully exited the strip
  bool tailStarted[2];    // One-time init guard per tail

  CelestialAscensionSlaveEffect()
    : Effect(7) {
    Serial.println("Init Ascension Effect");
    phaseStart = millis();
    phase = CAS_PROPAGATING;

    // Initialise wing state
    for (uint8_t i = 0; i < 2; i++) {
      wingRipple[i] = 0;
      wingPosFP[i] = 0;
      wingSpeedFP[i] = CA_WING_BASE_SPEED_FP;
      wingDone[i] = false;
    }

    // Initialise tail state
    for (uint8_t i = 0; i < 2; i++) {
      tailPosFP[i] = 0;
      tailActive[i] = false;
      tailDone[i] = false;
      tailStarted[i] = false;
    }
  }

  // ─────────────────────────────────────────────
  void update() override {
    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    if (phase == CAS_PROPAGATING) {
      // ── Advance wing positions (fixed-point, 8ms tick) ───────────────────
      for (uint8_t i = 0; i < 2; i++) {
        if (wingDone[i]) continue;

        // Advance: position += speed_fp / 8  (8ms tick compensation)
        // We accumulate in 64ths-of-a-pixel; strip_len in FP = NUM_LEDS * 64
        wingPosFP[i] += (wingSpeedFP[i] >> 3);  // >>3 = /8 for 125Hz tick

        uint16_t maxFP = (uint16_t)NUM_LEDS * 64;
        if (wingPosFP[i] >= maxFP) {
          // Completed this ripple pass — advance to next
          wingRipple[i]++;
          if (wingRipple[i] >= CA_WING_RIPPLE_COUNT) {
            wingDone[i] = true;
          } else {
            // Accelerate: double speed each successive ripple
            wingSpeedFP[i] = (uint16_t)(wingSpeedFP[i] << CA_WING_ACCEL_SHIFT);
            // Clamp to avoid overflow / unreasonable speed
            if (wingSpeedFP[i] > 3000) wingSpeedFP[i] = 3000;
            wingPosFP[i] = 0;
          }
        }
      }

      // ── Activate tails after delay window ────────────────────────────────
      for (uint8_t i = 0; i < 2; i++) {
        if (!tailActive[i] && elapsed >= CA_TAIL_DELAY_MS) {
          tailActive[i] = true;
          tailStarted[i] = true;
        }
        if (!tailActive[i] || tailDone[i]) continue;

        uint16_t speed = (i == 0) ? CA_TAIL2_SPEED_FP : CA_TAIL3_SPEED_FP;
        tailPosFP[i] += (speed >> 3);

        uint16_t maxFP = (uint16_t)NUM_LEDS * 64;
        if (tailPosFP[i] >= maxFP) {
          tailPosFP[i] = maxFP;
          tailDone[i] = true;
        }
      }

      // ── Transition to settle once all propagation is complete ─────────────
      bool allWingsDone = wingDone[0] && wingDone[1];
      bool allTailsDone = tailDone[0] && tailDone[1];
      if (allWingsDone && allTailsDone) {
        phase = CAS_SETTLE;
        phaseStart = now;
      }
    } else if (phase == CAS_SETTLE) {
      if (elapsed >= CA_SLAVE_SETTLE_MS) {
        phase = CAS_DONE;
        done = true;
      }
    }
  }

  // ─────────────────────────────────────────────
  void draw(CRGB* leds_1, CRGB* leds_2, CRGB* leds_3, CRGB* leds_4) override {
    CRGB* stripPtrs[CA_SLAVE_STRIP_COUNT] = { leds_1, leds_2, leds_3, leds_4 };
    const uint8_t stripLens[CA_SLAVE_STRIP_COUNT] = {
      NUM_LEDS, NUM_LEDS, NUM_LEDS, NUM_LEDS
    };

    // ════════════════════════════════════════
    // PROPAGATING
    // ════════════════════════════════════════
    if (phase == CAS_PROPAGATING) {

      // ── Wings (strips 0 & 1) ────────────────────────────────────────────
      for (uint8_t i = 0; i < 2; i++) {
        if (wingDone[i]) continue;
        CRGB* strip = stripPtrs[i];
        uint8_t len = stripLens[i];

        // Convert FP position to integer pixel index
        uint8_t front = (uint8_t)(wingPosFP[i] >> 6);  // >>6 = /64
        if (front >= len) front = len - 1;

        // Sub-pixel fractional blend: 0–63 range → 0–255
        uint8_t frac = (uint8_t)((wingPosFP[i] & 0x3F) << 2);  // scale to 8-bit

        // ── Render trailing heat gradient ────────────────────────────────
        // Layer 0 (head): white-hot core, anti-aliased with fractional blend
        _renderWingPixel(strip, len, front, 255, 255, 200, 255);       // full pixel
        _renderWingPixel(strip, len, front + 1, 255, 255, 200, frac);  // fractional overhang

        // Layer -1: bright orange
        if (front >= 1)
          _renderWingPixel(strip, len, front - 1, 255, 130, 0, 255);

        // Layer -2: amber
        if (front >= 2)
          _renderWingPixel(strip, len, front - 2, 255, 70, 0, 220);

        // Layer -3: deep orange
        if (front >= 3)
          _renderWingPixel(strip, len, front - 3, 200, 30, 0, 160);

        // Layer -4: charcoal red trailing ember
        if (front >= 4)
          _renderWingPixel(strip, len, front - 4, 100, 5, 0, 100);
      }

      // ── Tails (strips 2 & 3) ────────────────────────────────────────────
      for (uint8_t i = 0; i < 2; i++) {
        if (!tailActive[i]) continue;
        CRGB* strip = stripPtrs[i + 2];
        uint8_t len = stripLens[i + 2];

        uint8_t front = (uint8_t)(tailPosFP[i] >> 6);
        if (front >= len) front = len - 1;
        uint8_t frac = (uint8_t)((tailPosFP[i] & 0x3F) << 2);

        // Tails use a wider, heavier gradient to simulate trailing heat mass
        _renderWingPixel(strip, len, front, 255, 255, 200, 255);       // white-hot tip
        _renderWingPixel(strip, len, front + 1, 255, 255, 200, frac);  // fractional

        if (front >= 1)
          _renderWingPixel(strip, len, front - 1, 255, 120, 0, 255);

        if (front >= 2)
          _renderWingPixel(strip, len, front - 2, 255, 60, 0, 230);

        if (front >= 3)
          _renderWingPixel(strip, len, front - 3, 220, 20, 0, 190);

        if (front >= 4)
          _renderWingPixel(strip, len, front - 4, 160, 8, 0, 140);

        if (front >= 5)
          _renderWingPixel(strip, len, front - 5, 80, 2, 0, 90);

        // Charcoal red deep tail: fade out over remaining trailing pixels
        if (front >= 6)
          _renderWingPixel(strip, len, front - 6, 40, 0, 0, 60);
      }
    }

    // ════════════════════════════════════════
    // SETTLE — warm residual glow fades across all strips
    // Ultra-fast bitwise approximation: glow >> 1 for green channel
    // ════════════════════════════════════════
    else if (phase == CAS_SETTLE) {
      uint32_t elapsed = millis() - phaseStart;
      uint8_t factor = (uint8_t)((elapsed * 255UL) / CA_SLAVE_SETTLE_MS);
      uint8_t glow = scale8(50, 255 - factor);  // warm amber residual, decays to 0

      if (glow == 0) return;

      for (uint8_t s = 0; s < CA_SLAVE_STRIP_COUNT; s++) {
        CRGB* strip = stripPtrs[s];
        uint8_t len = stripLens[s];
        for (uint8_t p = 0; p < len; p++) {
          strip[p].r = qadd8(strip[p].r, glow);
          strip[p].g = qadd8(strip[p].g, glow >> 1);  // zero-cost bitwise half
          // no blue — stays in warm ember register
        }
      }
    }
  }

private:
  // ── Additive pixel writer with per-call alpha scaling ────────────────────
  // Safely bounds-checks against strip length before writing.
  inline void _renderWingPixel(CRGB* strip, uint8_t len,
                               uint8_t pos,
                               uint8_t r, uint8_t g, uint8_t b,
                               uint8_t alpha) {
    if (pos >= len) return;
    strip[pos].r = qadd8(strip[pos].r, scale8(r, alpha));
    strip[pos].g = qadd8(strip[pos].g, scale8(g, alpha));
    strip[pos].b = qadd8(strip[pos].b, scale8(b, alpha));
  }
};
