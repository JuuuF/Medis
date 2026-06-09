#pragma once

#include "config.h"

#include <FastLED.h>

#define PRIDE_REVEAL_MS 3000      // Phase 0: Curtain drops from neck to tail
#define PRIDE_HOLD_MS 2000        // Phase 1: Static flag held before wings start
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

class PrideCascadeSlaveEffect : public Effect {
public:
  PridePhase phase;
  uint32_t phaseStart;

  PrideCascadeSlaveEffect()
    : Effect(6) {
    phase = PRIDE_CYCLING;
    phaseStart = millis();
    done = false;
  }

  void update() override {
    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    switch (phase) {
      case PRIDE_CYCLING:
        if (elapsed >= PRIDE_WING_CYCLE_MS) {
          phase = PRIDE_SPARKLING;
          phaseStart = now;
        }
        break;

      case PRIDE_SPARKLING:  // Smooth trailing fade-out phase
        if (elapsed >= PRIDE_SPARKLE_MS) {
          phase = PRIDE_DONE;
          done = true;
        }
        break;

      default:
        break;
    }
  }

  void draw(CRGB *leds_1, CRGB *leds_2, CRGB *leds_3, CRGB *leds_4) override {
    if (phase == PRIDE_DONE) return;

    uint32_t nowTime = millis();

    // ── 1. Continuous Wave Progress ─────────────────────────────────────────
    uint32_t waveElapsed = (phase == PRIDE_CYCLING) ? (nowTime - phaseStart) : (PRIDE_WING_CYCLE_MS + (nowTime - phaseStart));
    uint16_t waveProgress = (waveElapsed * (256 * PRIDE_CYCLES_WINGS)) / PRIDE_WING_CYCLE_MS;

    // ── 2. Emerge & Fade Envelopes ──────────────────────────────────────────
    uint8_t revealWall = 255;
    uint8_t globalAlpha = 255;

    if (phase == PRIDE_CYCLING) {
      uint32_t introElapsed = nowTime - phaseStart;
      const uint16_t INTRO_MS = 1000;  // 1 second to fully expand out from base to tip
      if (introElapsed < INTRO_MS) {
        revealWall = (introElapsed * 255) / INTRO_MS;
      }
    } else if (phase == PRIDE_SPARKLING) {
      uint32_t fadeElapsed = nowTime - phaseStart;
      if (fadeElapsed >= PRIDE_SPARKLE_MS) return;
      globalAlpha = lerp8by8(255, 0, (fadeElapsed * 255) / PRIDE_SPARKLE_MS);
    }

    // ── 3. Strip Rendering Loop ─────────────────────────────────────────────
    for (uint8_t s = 0; s < 4; s++) {
      CRGB *targetStrip = nullptr;
      if (s == 0) targetStrip = leds_1;
      else if (s == 1) targetStrip = leds_2;
      else if (s == 2) targetStrip = leds_3;
      else if (s == 3) targetStrip = leds_4;

      if (targetStrip == nullptr) continue;

      for (uint8_t px = 0; px < 40; px++) {
        // Map the current pixel position to a 0-255 scale to check against the reveal wall
        uint8_t pxPosition = (px * 255) / 39;

        // If the emerging front hasn't reached this pixel yet, skip drawing it
        if (phase == PRIDE_CYCLING && pxPosition > revealWall) continue;

        // Calculate continuous wave color
        uint8_t colorIndex = waveProgress - (px * 6);
        uint8_t section = colorIndex / 42;
        uint8_t offset = (colorIndex % 42) * 6;
        CRGB finalColor;

        switch (section) {
          case 0: finalColor = CRGB(255, 0, 0); break;
          case 1: finalColor = CRGB(255, qadd8(0, offset), 0); break;
          case 2: finalColor = CRGB(0, 255, 0); break;
          case 3: finalColor = CRGB(0, 0, 255); break;
          case 4: finalColor = CRGB(75, 0, 130); break;
          default: finalColor = CRGB(238, 130, 238); break;
        }

        // Apply a feathering soft fade to the edge of the emerging reveal wall
        if (phase == PRIDE_CYCLING && revealWall < 255) {
          uint8_t edgeDistance = revealWall - pxPosition;
          if (edgeDistance < 40) {  // Apply a soft 40-unit fade gradient at the front line
            finalColor.nscale8((edgeDistance * 255) / 40);
          }
        }

        // Apply the overall master phase fade-out modifier
        finalColor.nscale8(globalAlpha);

        // Blend onto active wing index pointers safely
        targetStrip[px].r = qadd8(targetStrip[px].r, finalColor.r);
        targetStrip[px].g = qadd8(targetStrip[px].g, finalColor.g);
        targetStrip[px].b = qadd8(targetStrip[px].b, finalColor.b);
      }
    }
  }
};