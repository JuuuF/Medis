#pragma once

#include "config.h"
#include "effects.h"
#include "mapping.h"
#include "communication.h"
#include <FastLED.h>

// ─────────────────────────────────────────────
// Hardcoded Timers & Structural Constants
#define CA_THERMAL_STRETCH_MS 4500                              
#define CA_SURGE_COUNT 4                                        
#define CA_SURGE_CYCLE_MS 700                                   
#define CA_SURGE_TOTAL_MS (CA_SURGE_COUNT * CA_SURGE_CYCLE_MS)  

// Stretch phase spatial constants - adjusted for inverted fire profile
#define CA_STRETCH_DARK_ROWS 4    // Attenuates rows closest to the neck base
#define CA_MIN_BOTTOM_SCALE 40   

// Surge phase rendering constants
#define CA_CROWN_FLASH_ROWS 2     
#define CA_SURGE_WASH_ALPHA 235   
#define CA_SURGE_WASH_FALLOFF 64  

enum CAPhase : uint8_t {
  CA_THERMAL_STRETCH = 0,
  CA_SURGE_CYCLE,
  CA_DONE
};

// ═════════════════════════════════════════════
// MASTER LEADER CLASS (Aligned to Inverted Fire Coordinates)
// ═════════════════════════════════════════════
class CelestialAscensionEffect : public Effect {
public:
  CAPhase phase;
  uint32_t phaseStart;

  uint8_t surgeIndex;   
  bool broadcastFired;  
  uint8_t stretchRamp;  

  CelestialAscensionEffect()
    : Effect(7) {
    phaseStart = millis();
    phase = CA_THERMAL_STRETCH;
    surgeIndex = 0;
    broadcastFired = false;
    stretchRamp = 0;
  }

  void update() override {
    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    switch (phase) {
      case CA_THERMAL_STRETCH:
        stretchRamp = (elapsed >= CA_THERMAL_STRETCH_MS)
                        ? 255
                        : (uint8_t)((elapsed * 255UL) / CA_THERMAL_STRETCH_MS);

        if (elapsed >= CA_THERMAL_STRETCH_MS) {
          phase = CA_SURGE_CYCLE;
          phaseStart = now;
          surgeIndex = 0;
        }
        break;

      case CA_SURGE_CYCLE:
        {
          uint8_t newSurge = (uint8_t)(elapsed / CA_SURGE_CYCLE_MS);
          if (newSurge >= CA_SURGE_COUNT) {
            phase = CA_DONE;
            done = true;
            break;
          }

          if (!broadcastFired) {
            broadcastEffect(effectID);
            broadcastFired = true;
          }

          surgeIndex = newSurge;
          break;
        }
      default:
        break;
    }
  }

  void draw(CRGB* leds) override {
    switch (phase) {

      // ════════════════════════════════════════
      // ALIGNED THERMAL STRETCH
      // ════════════════════════════════════════
      case CA_THERMAL_STRETCH:
        {
          for (uint8_t y = 0; y < FLAME_HEIGHT; y++) {
            // Normalized spatial scaling constant (0-255)
            uint8_t rowFrac = (uint8_t)(255UL * y / (FLAME_HEIGHT - 1));
            
            // CRITICAL FIX: The fire core is at the top (rowFrac = 255).
            // To make the fire "stretch upward," we must sap the heavy core at the top
            // and forcefully amplify the tail/tips at the bottom (y = 0).
            uint8_t suppress = scale8(rowFrac, stretchRamp); 

            // Lift amplification targets the lower tips, pulling them upward
            uint8_t rawLift = 255 - rowFrac;
            uint8_t liftAlpha = scale8(scale8(rawLift, rawLift), stretchRamp);

            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
              uint16_t idx = XY(x, y);

              // ── Core Suppression (Sapping the Neck Base) ──
              if (suppress > 0) {
                uint8_t dimFactor = 255 - scale8(suppress, 190); 
                leds[idx].nscale8_video(dimFactor); 
              }

              // ── Lift Overlay (Injecting high energy down into the tips) ──
              if (liftAlpha > 0) {
                leds[idx].r = qadd8(leds[idx].r, scale8(240, liftAlpha));
                leds[idx].g = qadd8(leds[idx].g, scale8(130, liftAlpha));
              }
            }
          }
          break;
        }

      // ════════════════════════════════════════
      // ALIGNED SURGE CYCLE
      // ════════════════════════════════════════
      case CA_SURGE_CYCLE:
        {
          uint32_t elapsed = millis() - phaseStart;
          uint32_t cycleElapsed = elapsed - (uint32_t)surgeIndex * CA_SURGE_CYCLE_MS;
          uint8_t t = (uint8_t)((cycleElapsed * 255UL) / CA_SURGE_CYCLE_MS);

          // Exponential surge front moving upward from tail (y=0) to neck (y=FLAME_HEIGHT-1)
          uint8_t surgeFront = scale8(t, t);  
          uint8_t baseGlow = scale8(45, (uint8_t)(surgeIndex * 64));

          for (uint8_t y = 0; y < FLAME_HEIGHT; y++) {
            uint8_t rowFrac = (uint8_t)(255UL * y / (FLAME_HEIGHT - 1));

            // Wide-envelope tracking math to ensure smooth wave travel over small rows
            int16_t dist = (int16_t)rowFrac - (int16_t)surgeFront;
            if (dist < 0) dist = -dist; 

            uint8_t washBrightness = 0;
            if (dist < CA_SURGE_WASH_FALLOFF) {
              uint8_t factor = (uint8_t)(((uint16_t)dist * 255) / CA_SURGE_WASH_FALLOFF);
              washBrightness = scale8(CA_SURGE_WASH_ALPHA, 255 - factor);
            }

            // Crown overexposure boundary (The Neck row right before head entry)
            bool isCrown = (y >= FLAME_HEIGHT - CA_CROWN_FLASH_ROWS);
            uint8_t crownAdd = 0;
            if (isCrown && t > 195) {
              crownAdd = scale8(255, (uint8_t)((t - 195) * 4)); 
            }

            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
              uint16_t idx = XY(x, y);

              // Maintain stabilized convective root zones at the generator neck rows
              if (y >= FLAME_HEIGHT - CA_STRETCH_DARK_ROWS) {
                leds[idx].nscale8_video(110); 
                leds[idx].r = qadd8(leds[idx].r, 40 + baseGlow);
                leds[idx].g = scale8(leds[idx].g, 40);
                leds[idx].b = 0;
                continue;
              }

              // Additive surge front implementation
              if (washBrightness > 0) {
                leds[idx].r = qadd8(leds[idx].r, washBrightness);
                leds[idx].g = qadd8(leds[idx].g, scale8(washBrightness, 110));
              }

              // Blinding white-hot flash overlay
              if (crownAdd > 0) {
                leds[idx].r = qadd8(leds[idx].r, crownAdd);
                leds[idx].g = qadd8(leds[idx].g, crownAdd);
                leds[idx].b = qadd8(leds[idx].b, scale8(crownAdd, 200));
              }
            }
          }

          // Spills flawlessly upward from the neck into the head matrix rows
          for (uint8_t y = FLAME_HEIGHT; y < MATRIX_HEIGHT; y++) {
            uint8_t headFrac = (uint8_t)(255UL * (y - FLAME_HEIGHT) / (HEAD_LEDS - 1));
            uint8_t haloAlpha = scale8(surgeFront, 255 - headFrac);
            
            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
              uint16_t idx = XY(x, y);
              leds[idx].r = qadd8(leds[idx].r, scale8(220, haloAlpha));
              leds[idx].g = qadd8(leds[idx].g, scale8(95, haloAlpha));
            }
          }
          break;
        }
      default:
        break;
    }
  }
};