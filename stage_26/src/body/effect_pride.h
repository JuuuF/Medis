#pragma once

#include "config.h"
#include <FastLED.h>

#define PRIDE_REVEAL_MS 3000      // Phase 0: Curtain drops from neck to tail
#define PRIDE_HOLD_MS 500         // Phase 1: Static flag held before wings start
#define PRIDE_WING_CYCLE_MS 4000  // Phase 2: Wing strips dynamically pulse
#define PRIDE_SPARKLE_MS 2500     // Phase 3: Sparkle away dissolve

#define PRIDE_CYCLES_WINGS 3
#define PRIDE_SUPPRESSION 240

enum PridePhase : uint8_t {
  PRIDE_REVEALING = 0,
  PRIDE_HOLDING = 1,
  PRIDE_CYCLING = 2,
  PRIDE_SPARKLING = 3,
  PRIDE_DONE = 4
};
class PrideCascadeEffect : public Effect {
public:
  PridePhase phase;
  uint32_t phaseStart;

  PrideCascadeEffect(uint8_t id)
    : Effect(id) {
    Serial.println("Init Pride Effect");
    phaseStart = millis();
    phase = PRIDE_REVEALING;
    done = false;
  }

  void update() override {
    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    switch (phase) {
      case PRIDE_REVEALING:
        if (elapsed >= PRIDE_REVEAL_MS) {
          phase = PRIDE_HOLDING;
          phaseStart = now;
        }
        break;

      case PRIDE_HOLDING:
        if (elapsed >= PRIDE_HOLD_MS) {
          phase = PRIDE_CYCLING;
          phaseStart = now;
          broadcastEffect(effectID);
        }
        break;

      case PRIDE_CYCLING:
        if (elapsed >= PRIDE_WING_CYCLE_MS) {
          phase = PRIDE_SPARKLING;
          phaseStart = now;
        }
        break;

      case PRIDE_SPARKLING:
        if (elapsed >= PRIDE_SPARKLE_MS) {
          phase = PRIDE_DONE;
          done = true;
        }
        break;

      default:
        break;
    }
  }

  void draw(CRGB* leds) override {
    uint32_t elapsed = millis() - phaseStart;
    uint8_t bodyHeight = MATRIX_HEIGHT - HEAD_LEDS;

    // ── 1. Render Isolated Phase Boundary Values ────────────────────────────
    uint16_t curtainY = 0;
    uint8_t localSuppression = PRIDE_SUPPRESSION;

    if (phase == PRIDE_REVEALING) {
      curtainY = bodyHeight - ((elapsed * bodyHeight) / PRIDE_REVEAL_MS);
    } else if (phase == PRIDE_SPARKLING) {
      localSuppression = lerp8by8(PRIDE_SUPPRESSION, 0, (elapsed * 255) / PRIDE_SPARKLE_MS);
    }

    // ── 2. Background Suppression (EXCLUDING HEAD) ──────────────────────────
    for (uint8_t y = 0; y < bodyHeight; y++) {
      if (y < curtainY) continue;
      for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        leds[XY(x, y)].nscale8_video(255 - localSuppression);
      }
    }

    // ── 3. Render Flag Layer (EXCLUDING HEAD) ───────────────────────────────
    for (uint8_t y = curtainY; y < bodyHeight; y++) {
      uint8_t colorIndex = ((bodyHeight - 1 - y) * 255) / bodyHeight;
      CRGB flagColor = computePrideColor(colorIndex);

      for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
        CRGB pixelColor = flagColor;

        if (phase == PRIDE_SPARKLING) {
          uint8_t pixelSeed = (uint8_t)(((x * 17) ^ (y * 31)) & 0xFF);
          uint32_t sparkleStart = (uint32_t)pixelSeed * (PRIDE_SPARKLE_MS - 600) / 255;

          if (elapsed >= sparkleStart) {
            uint32_t pxAge = elapsed - sparkleStart;
            const uint32_t FLASH_MS = 100;
            const uint32_t DECAY_MS = 500;

            if (pxAge < FLASH_MS) {
              pixelColor = blend(flagColor, CRGB::White, (pxAge * 255) / FLASH_MS);
            } else if (pxAge < (FLASH_MS + DECAY_MS)) {
              uint32_t decayElapsed = pxAge - FLASH_MS;
              uint8_t decayAlpha = (decayElapsed * 255) / DECAY_MS;
              CRGB passionColor = CHSV(scale8(colorIndex, 12), 255, 180);
              pixelColor = blend(CRGB::White, passionColor, decayAlpha);
              pixelColor.nscale8(lerp8by8(255, 0, decayAlpha));
            } else {
              continue;  // Exits to background fire
            }
          }
        }

        _blend(leds, x, y, pixelColor);
      }
    }
  }

private:
  CRGB computePrideColor(uint8_t index) {
    uint8_t section = index / 42;
    uint8_t offset = (index % 42) * 6;
    switch (section) {
      case 0: return CRGB(255, 0, 0);
      case 1: return CRGB(255, qadd8(0, offset), 0);
      case 2: return CRGB(0, 255, 0);
      case 3: return CRGB(0, 0, 255);
      case 4: return CRGB(75, 0, 130);
      default: return CRGB(238, 130, 238);
    }
  }

  void _blend(CRGB* leds, int8_t x, int8_t y, CRGB col) {
    uint16_t idx = XY(x, y);
    leds[idx].r = qadd8(leds[idx].r, col.r);
    leds[idx].g = qadd8(leds[idx].g, col.g);
    leds[idx].b = qadd8(leds[idx].b, col.b);
  }
};