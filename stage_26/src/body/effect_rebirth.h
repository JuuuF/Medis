#pragma once

#include "config.h"
#include "effects.h"
#include "communication.h"

#include <FastLED.h>

// ----------------------------------------------
// Parameters

#define REBIRTH_COLLAPSE_MS 15000  // ms: fire collapses inward
#define REBIRTH_DEAD_MS 3000       // ms: dead ember pulse, audience tension
#define REBIRTH_IGNITION_MS 150    // ms: white-hot ignition flash
#define REBIRTH_EXPANSION_MS 2000  // ms: explosive bloom expansion
#define REBIRTH_RECOVERY_MS 5000   // ms: fire returns stronger than before

#define REBIRTH_COLLAPSE_SPEED 200       // 0–255: how fast edges extinguish (higher = faster collapse)
#define REBIRTH_EMBER_PULSE_RATE 800     // ms: half-period of the dead-core dim pulse
#define REBIRTH_IGNITION_BRIGHTNESS 255  // peak white-hot brightness at ignition
#define REBIRTH_RECOVERY_INTENSITY 220   // additive brightness boost during recovery bloom
#define REBIRTH_SECONDARY_RIPPLES 3      // number of aftershock ripple waves during expansion

// Internal: radius (in matrix columns) of the surviving ember core
#define REBIRTH_CORE_RADIUS 2

// ----------------------------------------------
// Phase Enum

enum RebirthPhase : uint8_t {
  RP_COLLAPSE = 0,   // fire shrinks, edges die
  RP_DEAD = 1,       // dim pulsing ember core
  RP_IGNITION = 2,   // single white-hot flash
  RP_EXPANSION = 3,  // explosive bloom, fire overtakes body
  RP_RECOVERY = 4,   // fire settles stronger than before
  RP_DONE = 5
};

// ----------------------------------------------

class RebirthEffect : public Effect {
public:

  uint32_t phaseStart;
  RebirthPhase phase;

  // Collapse: per-LED extinction mask (0 = still alive, 1 = snuffed)
  // We use a uint8_t "snuff progress" per column: columns die outside-in.
  uint8_t collapseEdge;  // current innermost column that has been snuffed (counts inward)

  // Dead ember: simple sine-style pulse state
  uint8_t emberPulseBrightness;
  bool emberPulseRising;

  // Expansion ripple tracking
  struct Ripple {
    bool alive;
    uint8_t radius;  // in columns from center
    uint8_t brightness;
  };
  Ripple ripples[REBIRTH_SECONDARY_RIPPLES];

  // ----------------------------------

  RebirthEffect()
    : Effect(4)  // effectID = 4; register a new ID for slaves
  {
    phaseStart = millis();
    phase = RP_COLLAPSE;
    collapseEdge = 0;
    emberPulseBrightness = 20;
    emberPulseRising = false;
    memset(ripples, 0, sizeof(ripples));
    broadcastEffect(effectID);
  }

  void update() override {

    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    switch (phase) {

      case RP_COLLAPSE:
        // Advance the extinction front based on elapsed time
        collapseEdge = (uint8_t)((uint32_t)elapsed * (MATRIX_WIDTH / 2) / REBIRTH_COLLAPSE_MS);
        if (collapseEdge > MATRIX_WIDTH / 2) collapseEdge = MATRIX_WIDTH / 2;

        if (elapsed >= REBIRTH_COLLAPSE_MS) {
          phase = RP_DEAD;
          phaseStart = now;
          collapseEdge = MATRIX_WIDTH / 2;  // everything outside core snuffed
        }
        break;

      case RP_DEAD:
        // Simple triangular pulse for ember breathing
        if (emberPulseRising) {
          emberPulseBrightness = qadd8(emberPulseBrightness, 2);
          if (emberPulseBrightness >= 55) emberPulseRising = false;
        } else {
          emberPulseBrightness = qsub8(emberPulseBrightness, 2);
          if (emberPulseBrightness <= 10) emberPulseRising = true;
        }

        if (elapsed >= REBIRTH_DEAD_MS) {
          phase = RP_IGNITION;
          phaseStart = now;
          // Tell slaves the rebirth is happening NOW
        }
        break;

      case RP_IGNITION:
        if (elapsed >= REBIRTH_IGNITION_MS) {
          phase = RP_EXPANSION;
          phaseStart = now;
          _spawnRipples();
        }
        break;

      case RP_EXPANSION:
        _updateRipples(elapsed);
        if (elapsed >= REBIRTH_EXPANSION_MS) {
          phase = RP_RECOVERY;
          phaseStart = now;
        }
        break;

      case RP_RECOVERY:
        if (elapsed >= REBIRTH_RECOVERY_MS) {
          phase = RP_DONE;
          done = true;
        }
        break;

      default:
        break;
    }
  }

  // ----------------------------------------------
  void draw(CRGB* leds) override {

    uint32_t elapsed = millis() - phaseStart;

    switch (phase) {

      // ── COLLAPSE: darken edges inward, shift remaining fire deep red ────────
      case RP_COLLAPSE:
        {
          uint8_t t = (uint8_t)((uint32_t)elapsed * 255 / REBIRTH_COLLAPSE_MS);

          // Increase red dominance, sap green & blue as fire weakens
          uint8_t greenSup = lerp8by8(0, 160, t);
          uint8_t dimMul = lerp8by8(255, 0, t);  // overall dim toward end

          uint8_t centerX = MATRIX_WIDTH / 2;
          uint8_t centerY = MATRIX_HEIGHT / 2;

          for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {

              uint16_t idx = (uint16_t)y * MATRIX_WIDTH + x;

              // Distance from center column (symmetric)
              uint8_t distFromCenter = (x >= centerX) ? (x - centerX) : (centerX - x);

              // If this column is beyond the live edge → snuff it
              if (distFromCenter >= (MATRIX_WIDTH / 2 - collapseEdge + REBIRTH_CORE_RADIUS)) {
                // Fade out progressively rather than hard-cut
                leds[idx].nscale8(180);
                leds[idx].g = qsub8(leds[idx].g, 40);
              } else {
                // Still alive: dim + redden based on global t
                leds[idx].nscale8(dimMul);
                leds[idx].g = qsub8(leds[idx].g, (greenSup * leds[idx].g) >> 8);
                leds[idx].b = qsub8(leds[idx].b, leds[idx].b >> 1);
              }
            }
          }
          break;
        }

      // ── DEAD EMBER: only tiny core glows, everything else black ─────────────
      case RP_DEAD:
        {
          uint8_t centerX = MATRIX_WIDTH / 2;
          uint8_t centerY = MATRIX_HEIGHT / 2;

          for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
              uint16_t idx = (uint16_t)y * MATRIX_WIDTH + x;

              uint8_t dx = (x >= centerX) ? (x - centerX) : (centerX - x);
              uint8_t dy = (y >= centerY) ? (y - centerY) : (centerY - y);

              if (dx <= REBIRTH_CORE_RADIUS && dy <= REBIRTH_CORE_RADIUS) {
                // Pulsing deep-red ember
                uint8_t b = emberPulseBrightness;
                // Falloff from exact center
                uint8_t dist = dx + dy;
                b = scale8(b, 255 - dist * 40);
                leds[idx] = CRGB(b, b >> 3, 0);  // deep red, almost no green
              } else {
                // FIX: Hard overwrite to black to forcefully smother the background fire
                leds[idx] = CRGB::Black;
              }
            }
          }
          break;
        }

      // ── IGNITION: white-hot flash from center, punchy ───────────────────────
      case RP_IGNITION:
        {
          // t: 0→255 over ignition window
          uint8_t t = (uint8_t)((uint32_t)elapsed * 255 / REBIRTH_IGNITION_MS);

          // Bell-shaped peak: brightest in the first half, quickly fades
          uint8_t flashT = (t < 128) ? (t << 1) : (255 - ((t - 128) << 1));
          uint8_t additive = scale8(REBIRTH_IGNITION_BRIGHTNESS, flashT);

          uint8_t centerX = MATRIX_WIDTH / 2;
          uint8_t centerY = MATRIX_HEIGHT / 2;

          for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
              uint16_t idx = (uint16_t)y * MATRIX_WIDTH + x;

              uint8_t dx = (x >= centerX) ? (x - centerX) : (centerX - x);
              uint8_t dy = (y >= centerY) ? (y - centerY) : (centerY - y);
              uint8_t dist = dx + dy;  // Manhattan, cheap & cheerful

              // White-hot at center, pure white (no color tint)
              uint8_t localB = qsub8(additive, dist * 18);
              leds[idx].r = qadd8(leds[idx].r, localB);
              leds[idx].g = qadd8(leds[idx].g, localB);
              leds[idx].b = qadd8(leds[idx].b, localB);
            }
          }
          break;
        }

      // ── EXPANSION: explosive golden bloom + secondary ripples ───────────────
      case RP_EXPANSION:
        {
          uint8_t t = (uint8_t)((uint32_t)elapsed * 255 / REBIRTH_EXPANSION_MS);

          // Golden-fire additive that fades over expansion
          uint8_t bloom = scale8(REBIRTH_RECOVERY_INTENSITY, 255 - t);

          for (uint16_t i = 0; i < NUM_LEDS; i++) {
            leds[i].r = qadd8(leds[i].r, bloom);
            leds[i].g = qadd8(leds[i].g, scale8(bloom, 160));  // golden tint
            leds[i].b = qadd8(leds[i].b, scale8(bloom, 30));
          }

          // Draw ripple rings
          _drawRipples(leds);
          break;
        }

      // ── RECOVERY: fire is back and burning hot; add a lingering warm boost ──
      case RP_RECOVERY:
        {
          uint8_t t = (uint8_t)((uint32_t)elapsed * 255 / REBIRTH_RECOVERY_MS);
          uint8_t glow = scale8(60, 255 - t);  // warm afterglow fades to nothing

          for (uint16_t i = 0; i < NUM_LEDS; i++) {
            leds[i].r = qadd8(leds[i].r, glow);
            leds[i].g = qadd8(leds[i].g, scale8(glow, 100));
          }
          break;
        }

      case RP_DONE:
      default:
        break;
    }
  }

private:

  void _spawnRipples() {
    for (uint8_t i = 0; i < REBIRTH_SECONDARY_RIPPLES; i++) {
      ripples[i].alive = true;
      ripples[i].radius = 0;
      ripples[i].brightness = 200 - i * 40;  // each successive ripple dimmer
    }
  }

  // Advance ripple radii; stagger launch by index so they don't all overlap
  void _updateRipples(uint32_t elapsed) {
    for (uint8_t i = 0; i < REBIRTH_SECONDARY_RIPPLES; i++) {
      if (!ripples[i].alive) continue;

      // Stagger: ripple i starts after i * 300 ms
      uint32_t rippleElapsed = (elapsed > (uint32_t)i * 300)
                                 ? elapsed - (uint32_t)i * 300
                                 : 0;

      // Radius grows from 0 to half matrix width over expansion time
      ripples[i].radius = (uint8_t)(rippleElapsed * (MATRIX_WIDTH / 2) / REBIRTH_EXPANSION_MS);

      // Fade brightness as it expands
      ripples[i].brightness = scale8(200 - i * 40,
                                     255 - (uint8_t)(rippleElapsed * 255 / REBIRTH_EXPANSION_MS));

      if (ripples[i].radius >= MATRIX_WIDTH / 2) {
        ripples[i].alive = false;
      }
    }
  }

  void _drawRipples(CRGB* leds) {
    uint8_t centerX = MATRIX_WIDTH / 2;
    uint8_t centerY = MATRIX_HEIGHT / 2;

    for (uint8_t ri = 0; ri < REBIRTH_SECONDARY_RIPPLES; ri++) {
      if (!ripples[ri].alive) continue;

      uint8_t R = ripples[ri].radius;
      uint8_t b = ripples[ri].brightness;

      for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
          uint8_t dx = (x >= centerX) ? (x - centerX) : (centerX - x);
          uint8_t dy = (y >= centerY) ? (y - centerY) : (centerY - y);
          uint8_t dist = dx + dy;

          // Draw pixels within ±1 of the ring radius
          if (dist >= R && dist <= R + 1) {
            uint16_t idx = (uint16_t)y * MATRIX_WIDTH + x;
            leds[idx].r = qadd8(leds[idx].r, b);
            leds[idx].g = qadd8(leds[idx].g, scale8(b, 160));
            leds[idx].b = qadd8(leds[idx].b, scale8(b, 40));
          }
        }
      }
    }
  }
};
