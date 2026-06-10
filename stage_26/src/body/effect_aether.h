#pragma once

#include "config.h"
#include "effects.h"
#include "mapping.h"
#include "communication.h"
#include <FastLED.h>

// ─────────────────────────────────────────────
// Phase Timers
#define AP_SHEAR_MS 5000    // Prism wave expands radially from center
#define AP_ECLIPSE_MS 1500  // Deep indigo void hold
#define AP_FLARE_MS 1000    // Solar flare sweeps bottom → top

// AP_PRISM_SHEAR tuning
// Radius at which the prism wave has fully consumed the whole matrix.
// Diagonal half-extent (Manhattan): MATRIX_WIDTH/2 + MATRIX_HEIGHT/2
// We scale it slightly past the corner so no pixel is ever skipped.
#define AP_SHEAR_RADIUS_MAX ((MATRIX_WIDTH / 2) + (MATRIX_HEIGHT / 2) + 2)

// AP_INDIGO_ECLIPSE tuning
// Red+Green are crushed to ≤ this fraction via nscale8 (≈10%)
#define AP_ECLIPSE_RG_FLOOR 26  // 26/255 ≈ 10%
// Blue ambient base during eclipse (low, cold, pulsing around this)
#define AP_ECLIPSE_BLUE_BASE 55
// Breathing swing amplitude added on top of base
#define AP_ECLIPSE_BLUE_SWING 30

// AP_SOLAR_FLARE tuning
// Additive overdrive values applied as the flare sweep passes a row
#define AP_FLARE_R_ADD 255
#define AP_FLARE_G_ADD 150

enum AethericPrismPhase : uint8_t {
  AP_PRISM_SHEAR = 0,
  AP_INDIGO_ECLIPSE,
  AP_SOLAR_FLARE,
  AP_DONE
};

// ═════════════════════════════════════════════
// LEADER EFFECT CLASS
// ═════════════════════════════════════════════
class AethericPrismEffect : public Effect {
public:
  AethericPrismPhase phase;
  uint32_t phaseStart;

  // SHEAR: current wave radius in fixed-point (8.8 → integer = shearRadiusFP >> 8)
  uint16_t shearRadiusFP;

  // ECLIPSE: simple tick counter for the sine-wave breathing on blue channel
  uint8_t eclipseTick;
  // Flag so broadcastEffect fires exactly once at Eclipse entry
  bool broadcastSent;

  // FLARE: which matrix row the flare sweep has reached (0 = floor, rising)
  uint8_t flareFrontRow;

  AethericPrismEffect(uint8_t id)
    : Effect(id) {
    Serial.println("Init AethericPrismEffect");
    phaseStart = millis();
    phase = AP_PRISM_SHEAR;
    shearRadiusFP = 0;
    eclipseTick = 0;
    broadcastSent = false;
    flareFrontRow = 0;
  }

  // ─────────────────────────────────────────
  void update() override {
    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    switch (phase) {

      case AP_PRISM_SHEAR:
        {
          // Advance radius: covers AP_SHEAR_RADIUS_MAX integer units over AP_SHEAR_MS.
          // Fixed-point 8.8: multiply target by 256 for fractional precision.
          shearRadiusFP = (uint16_t)((uint32_t)elapsed * (AP_SHEAR_RADIUS_MAX * 256UL) / AP_SHEAR_MS);

          if (elapsed >= AP_SHEAR_MS) {
            phase = AP_INDIGO_ECLIPSE;
            phaseStart = now;
            shearRadiusFP = AP_SHEAR_RADIUS_MAX * 256;  // clamp fully open
          }
          break;
        }

      case AP_INDIGO_ECLIPSE:
        {
          eclipseTick++;

          // Broadcast exactly once — at the very first tick of the eclipse,
          // which is the "eye" — the point of absolute desaturation.
          if (!broadcastSent) {
            broadcastEffect(effectID);
            broadcastSent = true;
          }

          if (elapsed >= AP_ECLIPSE_MS) {
            phase = AP_SOLAR_FLARE;
            phaseStart = now;
            flareFrontRow = 0;
          }
          break;
        }

      case AP_SOLAR_FLARE:
        {
          // Map elapsed → current sweep row (0 to MATRIX_HEIGHT-1)
          // The flare reaches the crown exactly when elapsed == AP_FLARE_MS
          flareFrontRow = (uint8_t)((uint32_t)elapsed * MATRIX_HEIGHT / AP_FLARE_MS);
          if (flareFrontRow >= MATRIX_HEIGHT) flareFrontRow = MATRIX_HEIGHT - 1;

          if (elapsed >= AP_FLARE_MS) {
            phase = AP_DONE;
            done = true;
          }
          break;
        }

      default: break;
    }
  }

  // ─────────────────────────────────────────
  void draw(CRGB* leds) override {
    if (phase == AP_DONE) return;

    const uint8_t cx = MATRIX_WIDTH / 2;
    const uint8_t cy = MATRIX_HEIGHT / 2;
    // Integer shear radius (upper 8 bits of the fixed-point value)
    const uint8_t shearRadius = (uint8_t)(shearRadiusFP >> 8);

    for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
      for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        uint16_t idx = XY(x, y);
        CRGB& pix = leds[idx];

        // Manhattan distance from center — no sqrt, pure integer
        uint8_t dist = (uint8_t)(abs((int8_t)x - (int8_t)cx) + abs((int8_t)y - (int8_t)cy));

        // ── Phase 1: PRISM SHEAR ────────────────────────────────────────
        if (phase == AP_PRISM_SHEAR) {
          if (dist > shearRadius) continue;  // Outside wave front — untouched

          // Sub-pixel anti-alias at the wave edge.
          // Pixels exactly at the frontier get a partial blend
          // so the wave front looks smooth rather than blocky.
          uint8_t edgeFrac = 255;  // fully inside
          if (dist == shearRadius) {
            // fractional part of the radius FP value
            edgeFrac = (uint8_t)(shearRadiusFP & 0xFF);
          }

          // Alchemical channel re-map:
          //   New R = old R  (keep intensity)
          //   New G = old G >> 1  (halved → deep magenta base)
          //   New B = old R + (255 - old B)  (red injected + blue inverted = luminous violet)
          uint8_t oldR = pix.r;
          uint8_t oldG = pix.g;
          uint8_t oldB = pix.b;

          CRGB transformed;
          transformed.r = oldR;
          transformed.g = oldG >> 1;
          transformed.b = qadd8(oldR, 255 - oldB);  // never overflows due to qadd8

          // Blend toward the transformed value proportional to edgeFrac
          if (edgeFrac == 255) {
            pix = transformed;
          } else {
            // lerp each channel independently (no CRGB copy constructor math)
            pix.r = lerp8by8(oldR, transformed.r, edgeFrac);
            pix.g = lerp8by8(oldG, transformed.g, edgeFrac);
            pix.b = lerp8by8(oldB, transformed.b, edgeFrac);
          }
          continue;
        }

        // ── Phase 2: INDIGO ECLIPSE ─────────────────────────────────────
        if (phase == AP_INDIGO_ECLIPSE) {
          // Crush R and G down to ≈10% of their current value
          pix.r = scale8(pix.r, AP_ECLIPSE_RG_FLOOR);
          pix.g = scale8(pix.g, AP_ECLIPSE_RG_FLOOR);

          // Blue: low ambient base + gentle sine breath via fast 8-bit sin
          // sin8(eclipseTick * 4) gives a full 0-255 sine wave at ~2 Hz
          uint8_t breathe = scale8(sin8(eclipseTick * 4), AP_ECLIPSE_BLUE_SWING);
          uint8_t bluePeak = AP_ECLIPSE_BLUE_BASE + breathe;

          // Force blue to the ambient level (non-additive — set directly
          // so the indigo void doesn't brighten beyond spec)
          pix.b = bluePeak;
          continue;
        }

        // ── Phase 3: SOLAR FLARE ────────────────────────────────────────
        if (phase == AP_SOLAR_FLARE) {
          // Rows below the rising flare front are fully restored and overdriven.
          // Rows above the front still show the fading indigo (handled below).
          if (y <= flareFrontRow) {
            // Additive golden overdrive — burns away any lingering blue cast
            pix.r = qadd8(pix.r, AP_FLARE_R_ADD);
            pix.g = qadd8(pix.g, AP_FLARE_G_ADD);
            // Blue suppressed: the gold wash dominates but don't hard-zero it
            // so the transition feels thermal rather than mechanical.
            pix.b = (pix.b > 30) ? pix.b - 30 : 0;
          } else {
            // Above the sweep: maintain residual eclipse look so the
            // contrast between "restored" and "void" rows is sharp and dramatic.
            pix.r = scale8(pix.r, AP_ECLIPSE_RG_FLOOR);
            pix.g = scale8(pix.g, AP_ECLIPSE_RG_FLOOR);
            pix.b = AP_ECLIPSE_BLUE_BASE;
          }
          continue;
        }
      }
    }
  }
};