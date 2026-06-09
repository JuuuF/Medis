#pragma once

#include "config.h"
#include "effects.h"
#include "mapping.h"
#include "communication.h"
#include <FastLED.h>

#define SB_COMPRESS_MS 5000
#define SB_HEAT_MS 2000
#define SB_RELEASE_MS 400
#define SB_FADE_MS 2000

enum SBPhase : uint8_t {
  SB_COMPRESS = 0,
  SB_HEAT,
  SB_RELEASE,
  SB_FADE,
  SB_DONE
};

class SacredBreathEffect : public Effect {
public:
  SBPhase phase;
  uint32_t phaseStart;
  uint8_t globalAlpha;
  bool broadcastSent;

  SacredBreathEffect(uint8_t id)
    : Effect(id) {
    Serial.println(F("Init SacredBreathEffect Simple"));
    phaseStart = millis();
    phase = SB_COMPRESS;
    globalAlpha = 0;
    broadcastSent = false;
  }

  void update() override {
    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    switch (phase) {
      case SB_COMPRESS:
        if (elapsed >= SB_COMPRESS_MS) {
          phase = SB_HEAT;
          phaseStart = now;
        }
        break;

      case SB_HEAT:
        if (elapsed >= SB_HEAT_MS) {
          phase = SB_RELEASE;
          phaseStart = now;
          if (!broadcastSent) {
            broadcastEffect(effectID);
            broadcastSent = true;
          }
          globalAlpha = 255;
        }
        break;

      case SB_RELEASE:
        {
          uint32_t ratio = (elapsed * 255UL) / SB_RELEASE_MS;
          globalAlpha = lerp8by8(255, 0, (ratio > 255) ? 255 : (uint8_t)ratio);

          if (elapsed >= SB_RELEASE_MS) {
            phase = SB_FADE;
            phaseStart = now;
            globalAlpha = 255;
          }
          break;
        }

      case SB_FADE:
        {
          uint32_t ratio = (elapsed * 255UL) / SB_FADE_MS;
          globalAlpha = lerp8by8(255, 0, (ratio > 255) ? 255 : (uint8_t)ratio);

          if (elapsed >= SB_FADE_MS) {
            phase = SB_DONE;
            done = true;
          }
          break;
        }

      default:
        break;
    }
  }

  void draw(CRGB* leds) override {
    uint32_t elapsed = millis() - phaseStart;

    switch (phase) {

      // ── Phase 1: Simple border fade (Collapses fire inward) ──────────────
      case SB_COMPRESS:
        {
          uint32_t timeRatio = (elapsed * 255UL) / SB_COMPRESS_MS;
          uint8_t ramp = (timeRatio > 255) ? 255 : (uint8_t)timeRatio;
          // Scale down the dimming impact slightly so it remains organic
          uint8_t dimFactor = lerp8by8(255, 40, ramp);

          for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
              // If the pixel is on the outer layout perimeter, dim it over time
              if (x == 0 || x == MATRIX_WIDTH - 1 || y == 0 || y == MATRIX_HEIGHT - 1) {
                leds[_getSnakeXY(x, y)].nscale8(dimFactor);
              }
            }
          }
          break;
        }

      // ── Phase 2: Simple golden heat push ─────────────────────────────────
      case SB_HEAT:
        {
          uint32_t timeRatio = (elapsed * 255UL) / SB_HEAT_MS;
          uint8_t heatRamp = (timeRatio > 255) ? 255 : (uint8_t)timeRatio;
          uint8_t boost = scale8(heatRamp, 80);

          for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
              uint16_t idx = _getSnakeXY(x, y);

              // Keep border dim hold active
              if (x == 0 || x == MATRIX_WIDTH - 1 || y == 0 || y == MATRIX_HEIGHT - 1) {
                leds[idx].nscale8(40);
              }

              // Simple color space shifting logic
              leds[idx].r = qadd8(leds[idx].r, boost);
              leds[idx].g = qadd8(leds[idx].g, boost >> 1);
              leds[idx].b = qsub8(leds[idx].b, scale8(heatRamp, 120));
            }
          }
          break;
        }

      // ── Phase 3: Global additive flash ───────────────────────────────────
      case SB_RELEASE:
        {
          if (globalAlpha == 0) break;

          for (uint8_t i = 0; i < MATRIX_WIDTH * MATRIX_HEIGHT; i++) {
            leds[i].r = qadd8(leds[i].r, globalAlpha);
            leds[i].g = qadd8(leds[i].g, globalAlpha);
            leds[i].b = qadd8(leds[i].b, globalAlpha);
          }
          break;
        }

      // ── Phase 4: Smooth amber settle wash ────────────────────────────────
      case SB_FADE:
        {
          if (globalAlpha == 0) break;
          uint8_t warmR = scale8(globalAlpha, 90);
          uint8_t warmG = scale8(globalAlpha, 45);

          for (uint8_t i = 0; i < MATRIX_WIDTH * MATRIX_HEIGHT; i++) {
            leds[i].r = qadd8(leds[i].r, warmR);
            leds[i].g = qadd8(leds[i].g, warmG);
          }
          break;
        }

      default:
        break;
    }
  }

private:
  inline uint16_t _getSnakeXY(uint8_t x, uint8_t y) {
    if (x >= MATRIX_WIDTH) x = MATRIX_WIDTH - 1;
    if (y >= MATRIX_HEIGHT) y = MATRIX_HEIGHT - 1;

    uint8_t phys_y = (MATRIX_HEIGHT - 1) - y;

    if (x % 2 == 0) {
      return (uint16_t)x * MATRIX_HEIGHT + phys_y;
    }
    return (uint16_t)x * MATRIX_HEIGHT + ((MATRIX_HEIGHT - 1) - phys_y);
  }
};