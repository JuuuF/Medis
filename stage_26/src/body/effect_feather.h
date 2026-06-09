#include "fl/gfx/colorutils.h"
#pragma once

#include "config.h"
#include "effects.h"
#include "mapping.h"
#include "communication.h"

#include <FastLED.h>

// ─────────────────────────────────────────────
// Parameters

#define FC_GLOW_MS 5000   // ms: body outline fades in
#define FC_HOLD_MS 2500   // ms: outline held before pulses
#define FC_PULSE_MS 1000  // ms: pulses travel outward + broadcast
#define FC_FADE_MS 1000   // ms: everything fades out

#define FC_OUTLINE_BRIGHTNESS 200  // peak brightness of body outline glow
#define FC_OUTLINE_HUE 30          // warm amber-gold
#define FC_OUTLINE_SAT 180

#define FC_PULSE_COUNT 3  // simultaneous pulses leaving torso
#define FC_PULSE_HUE 28   // slightly warmer than outline
#define FC_PULSE_WIDTH 2  // half-width in pixels

// ─────────────────────────────────────────────
// Phase enum

enum FCPhase : uint8_t {
  FC_GLOWING = 0,
  FC_HOLDING = 1,
  FC_PULSING = 2,
  FC_FADING = 3,
  FC_DONE = 4
};

// ─────────────────────────────────────────────
// A single pulse travelling from chest outward on the matrix
struct FCPulse {
  bool alive;
  uint8_t cx, cy;     // center origin of this pulse
  uint16_t radius16;  // radius scaled by 256 for smooth expansion
  uint8_t speed;      // expansion speed per frame
  uint8_t brightness;
};

// ═════════════════════════════════════════════
// MASTER EFFECT  (body + head matrix)
// ═════════════════════════════════════════════

class FeatherCascadeEffect : public Effect {
public:
  FCPhase phase;
  uint32_t phaseStart;

  FCPulse pulses[FC_PULSE_COUNT];

  FeatherCascadeEffect()
    : Effect(5)  // effectID = 5 — assign next free ID in your project
  {
    phaseStart = millis();
    phase = FC_GLOWING;
    memset(pulses, 0, sizeof(pulses));
  }

  // ── update ─────────────────────────────────
  void update() override {
    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    switch (phase) {

      case FC_GLOWING:
        if (elapsed >= FC_GLOW_MS) {
          phase = FC_HOLDING;
          phaseStart = now;
        }
        break;

      case FC_HOLDING:
        if (elapsed >= FC_HOLD_MS) {
          phase = FC_PULSING;
          phaseStart = now;
          _spawnPulses();
          broadcastEffect(effectID);
        }
        break;

      case FC_PULSING:
        _stepPulses();
        if (elapsed >= FC_PULSE_MS) {
          phase = FC_FADING;
          phaseStart = now;
        }
        break;

      case FC_FADING:
        _stepPulses();
        if (elapsed >= FC_FADE_MS) {
          phase = FC_DONE;
          done = true;
        }
        break;

      default:
        break;
    }
  }

  // ── draw ───────────────────────────────────
  void draw(CRGB* leds) override {
    uint32_t elapsed = millis() - phaseStart;

    // Global layer alpha for the fade-out phase
    uint8_t layerAlpha = 255;
    if (phase == FC_FADING) {
      layerAlpha = lerp8by8(255, 0,
                            (uint8_t)((uint32_t)elapsed * 255 / FC_FADE_MS));
    }

    // ── 0. Calculate Outline Brightness & Suppress Background ─────────────────
    uint8_t outlineBright = 0;

    if (phase == FC_GLOWING) {
      outlineBright = (uint8_t)((uint32_t)elapsed * FC_OUTLINE_BRIGHTNESS / FC_GLOW_MS);
    } else if (phase == FC_HOLDING || phase == FC_PULSING) {
      outlineBright = FC_OUTLINE_BRIGHTNESS;
    } else if (phase == FC_FADING) {
      outlineBright = scale8(FC_OUTLINE_BRIGHTNESS, layerAlpha);
    }

    // CRITICAL FIX: Only suppress the background fire effect during the intro/hold/pulse stages.
    // If we are in FC_FADING, stop eating the background so the active graphics can fade out cleanly.
    if (outlineBright > 0 && phase != FC_FADING) {
      uint8_t fireRetentionFactor = 255 - outlineBright;
      for (uint16_t i = 0; i < (MATRIX_WIDTH * MATRIX_HEIGHT); i++) {
        leds[i].nscale8_video(fireRetentionFactor);
      }
    }

    // ── 1. Body outline glow ─────────────────
    if (outlineBright > 0) {
      CHSV hsv(FC_OUTLINE_HUE, FC_OUTLINE_SAT, outlineBright);
      CRGB col;
      hsv2rgb_rainbow(hsv, col);
      _drawOutline(leds, col);
    }

    // ── 2. Torso pulses ──────────────────────
    if (phase == FC_PULSING || phase == FC_FADING) {
      for (uint8_t i = 0; i < FC_PULSE_COUNT; i++) {
        if (!pulses[i].alive) continue;

        uint8_t b = scale8(pulses[i].brightness, layerAlpha);
        CHSV hsv(FC_PULSE_HUE, 220, b);
        CRGB col;
        hsv2rgb_rainbow(hsv, col);

        // Current expansion radius in pixel units
        uint8_t currentRadius = pulses[i].radius16 >> 8;

        // Scan the matrix to draw pixels within the pulse width
        for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
          for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
            // Calculate Euclidean or Manhattan distance from pulse center
            uint8_t dist = abs(x - pulses[i].cx) + abs(y - pulses[i].cy);

            // If the pixel is sitting on the expanding wave front
            if (dist >= currentRadius && dist <= currentRadius + FC_PULSE_WIDTH) {
              // Apply a subtle fade to the trailing edge
              uint8_t falloff = 255 - ((dist - currentRadius) * (255 / (FC_PULSE_WIDTH + 1)));
              CRGB dimmed = col;
              dimmed.nscale8(falloff);

              _blend(leds, x, y, dimmed);
            }
          }
        }
      }
    }
  }

  // ─────────────────────────────────────────
private:

  // Draw a 1-pixel-wide outline around the body and head regions.
  // Walks the perimeter of the matrix and lights edge pixels.
  void _drawOutline(CRGB* leds, CRGB col) {
    // Top row (head top)
    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
      _blend(leds, x, 0, col);
    }
    // Bottom row (tail root)
    for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
      _blend(leds, x, MATRIX_HEIGHT - 1, col);
    }
    // Left and right columns
    for (uint8_t y = 1; y < MATRIX_HEIGHT - 1; y++) {
      _blend(leds, 0, y, col);
      _blend(leds, MATRIX_WIDTH - 1, y, col);
    }
  }

  // Pulse dot with FC_PULSE_WIDTH halo
  void _drawPulseAt(CRGB* leds, uint8_t cx, uint8_t cy, CRGB col) {
    for (int8_t dy = -FC_PULSE_WIDTH; dy <= FC_PULSE_WIDTH; dy++) {
      for (int8_t dx = -FC_PULSE_WIDTH; dx <= FC_PULSE_WIDTH; dx++) {
        uint8_t dist = abs(dx) + abs(dy);  // Manhattan distance
        if (dist > FC_PULSE_WIDTH) continue;
        uint8_t falloff = (dist == 0) ? 255 : (dist == 1 ? 140 : 60);
        CRGB dimmed = col;
        dimmed.nscale8(falloff);
        _blend(leds, (int8_t)cx + dx, (int8_t)cy + dy, dimmed);
      }
    }
  }

  void _blend(CRGB* leds, int8_t x, int8_t y, CRGB col) {
    if (x < 0 || x >= MATRIX_WIDTH) return;
    if (y < 0 || y >= MATRIX_HEIGHT) return;
    uint16_t idx = (uint16_t)y * MATRIX_WIDTH + x;
    leds[idx].r = qadd8(leds[idx].r, col.r);
    leds[idx].g = qadd8(leds[idx].g, col.g);
    leds[idx].b = qadd8(leds[idx].b, col.b);
  }

  // Spawn pulses at the chest centre, spread slightly
  void _spawnPulses() {
    uint8_t cx = MATRIX_WIDTH / 2;
    uint8_t cy = MATRIX_HEIGHT / 2;  // Center of entire matrix, or adjust to your chest center

    for (uint8_t i = 0; i < FC_PULSE_COUNT; i++) {
      pulses[i].alive = true;
      pulses[i].cx = cx;
      pulses[i].cy = cy;
      pulses[i].radius16 = 0;
      // Stagger expansion speeds or initial delays so they separate
      pulses[i].speed = random8(40, 70);
      pulses[i].brightness = 255;
    }
  }

  // Pulses drift upward (toward wing roots) and fade
  void _stepPulses() {
    for (uint8_t i = 0; i < FC_PULSE_COUNT; i++) {
      if (!pulses[i].alive) continue;

      pulses[i].radius16 += pulses[i].speed;

      // Calculate max possible diagonal radius to know when it leaves the screen
      uint16_t maxRadius = (MATRIX_WIDTH + MATRIX_HEIGHT) * 256;

      if (pulses[i].radius16 >= maxRadius) {
        pulses[i].alive = false;
      } else {
        // Fade out smoothly as it expands
        pulses[i].brightness = qsub8(255, (pulses[i].radius16 * 255) / maxRadius);
        if (pulses[i].brightness == 0) pulses[i].alive = false;
      }
    }
  }
};