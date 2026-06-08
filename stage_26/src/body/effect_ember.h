#pragma once

#include "config.h"
#include "effects.h"
#include "mapping.h"
#include "communication.h"

#include <FastLED.h>

// ----------------------------------------------
// Parameters

#define EMBER_CHARGE_MS 10000    // ms: inhale compression duration
#define EMBER_HOLD_MS 100        // ms: hold at peak pressure
#define EMBER_BURST_MS 350       // ms: golden-white burst duration
#define EMBER_WAVE_MS 2000       // ms: heat wave travel time (root→tip)
#define EMBER_ASH_MS 1000        // ms: ember/ash fade after wave
#define EMBER_TAIL_DELAY_MS 300  // ms: tail strips lag behind wings

#define EMBER_CHARGE_BRIGHTNESS 20  // floor brightness during charge (0=black)
#define EMBER_BURST_BRIGHTNESS 200  // additive brightness at burst peak
#define EMBER_TRAIL_FADE 40         // trail decay subtracted per tick
#define EMBER_SPAWN_RATE 60         // 0–255 probability of ember spawn per pos
#define EMBER_COUNT 12              // max simultaneous embers per strip

// ----------------------------------------------
// Internal constants

#define EMBER_NUM_STRIPS 4

// ----------------------------------------------
// Phase Enum

enum EmberPhase : uint8_t {
  EP_CHARGE = 0,
  EP_HOLD = 1,
  EP_BURST = 2,
  EP_WAVE = 3,
  EP_ASH = 4,
  EP_DONE = 5
};

struct EmberParticle {
  bool alive;
  uint8_t pos;         // position on strip
  uint8_t brightness;  // current brightness
  uint8_t hue;         // 0=red/orange, ~30=amber, ~45=gold
  int8_t vel;          // drift direction (+1 / -1 / 0)
};

// ----------------------------------------------

class EmberEffect : public Effect {
public:
  // Master state
  uint32_t phaseStart;
  EmberPhase phase;

  // Burst sparks ejected from the matrix during the burst phase
  struct BurstSpark {
    bool alive;
    uint8_t x, y;  // matrix coords
    uint8_t brightness;
    uint8_t decay;
  };
  static const uint8_t BURST_SPARK_COUNT = 20;
  BurstSpark burstSparks[BURST_SPARK_COUNT];

  // ----------------------------------
  EmberEffect()
    : Effect(3) {
    phaseStart = millis();
    phase = EP_CHARGE;
    memset(burstSparks, 0, sizeof(burstSparks));
  }
  // ----------------------------------
  void update() override {

    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    switch (phase) {

      case EP_CHARGE:
        if (elapsed >= EMBER_CHARGE_MS) {
          phase = EP_HOLD;
          phaseStart = now;
        }
        break;

      case EP_HOLD:
        if (elapsed >= EMBER_HOLD_MS) {
          phase = EP_BURST;
          phaseStart = now;
          _spawnBurstSparks();
          // broadcast effect info when changing to burst
          broadcastEffect(effectID);
        }
        break;

      case EP_BURST:
        _updateBurstSparks();
        if (elapsed >= EMBER_BURST_MS) {
          phase = EP_WAVE;
          phaseStart = now;
        }
        break;

      case EP_WAVE:
        // Matrix holds a lingering heat glow; slave handles propagation.
        if (elapsed >= EMBER_WAVE_MS) {
          phase = EP_ASH;
          phaseStart = now;
        }
        break;

      case EP_ASH:
        if (elapsed >= EMBER_ASH_MS) {
          phase = EP_DONE;
          done = true;
        }
        break;

      default:
        break;
    }
  }
  // ----------------------------------
  void draw(CRGB* leds) override {

    uint32_t elapsed = millis() - phaseStart;

    switch (phase) {

      // ── CHARGE: dim the fire, shift it redder, simulate heat gathering ──────
      case EP_CHARGE:
        {
          // t goes 0.0 → 1.0 over the charge phase
          uint8_t t = (uint8_t)((uint32_t)elapsed * 255 / EMBER_CHARGE_MS);

          // Brightness multiplier: linearly drops from 255 → EMBER_CHARGE_BRIGHTNESS
          uint8_t brightMul = lerp8by8(255, EMBER_CHARGE_BRIGHTNESS, t);

          // Color shift: pull green channel down so pixels redden
          uint8_t greenSup = lerp8by8(0, 80, t);  // suppress up to 80 from green

          for (uint16_t i = 0; i < NUM_LEDS; i++) {
            // Scale brightness
            leds[i].nscale8(brightMul);
            // Suppress green to shift warm-orange → deep red
            leds[i].g = qsub8(leds[i].g, (greenSup * leds[i].g) >> 8);
            // Faint blue bleed at very high charge (heat mirage)
            if (t > 200) {
              leds[i].b = qadd8(leds[i].b, (t - 200) >> 2);
            }
          }
          break;
        }

      // ── HOLD: sustain the compressed, dense-red core ──────────────────────
      case EP_HOLD:
        {
          uint8_t brightMul = EMBER_CHARGE_BRIGHTNESS;
          for (uint16_t i = 0; i < NUM_LEDS; i++) {
            leds[i].nscale8(brightMul);
            leds[i].g = qsub8(leds[i].g, (80 * leds[i].g) >> 8);
          }
          break;
        }

      // ── BURST: golden-white overexposure + ejected sparks ─────────────────
      case EP_BURST:
        {
          uint8_t t = (uint8_t)((uint32_t)elapsed * 255 / EMBER_BURST_MS);

          // Burst peak at t≈64, fades back
          uint8_t burstT = (t < 128) ? (t << 1) : (255 - ((t - 128) << 1));
          uint8_t additive = scale8(EMBER_BURST_BRIGHTNESS, burstT);

          for (uint16_t i = 0; i < NUM_LEDS; i++) {
            // Restore to full base, then add golden-white flash
            leds[i].r = qadd8(leds[i].r, additive);
            leds[i].g = qadd8(leds[i].g, scale8(additive, 180));  // gold tint
            leds[i].b = qadd8(leds[i].b, scale8(additive, 60));
          }

          // Draw burst sparks on top
          _drawBurstSparks(leds);
          break;
        }

      // ── WAVE: matrix returns toward normal fire; leave a lingering heat blush
      case EP_WAVE:
        {
          // Subtle warm afterglow that decays over the wave phase
          uint8_t t = (uint8_t)((uint32_t)elapsed * 255 / EMBER_WAVE_MS);
          uint8_t glow = scale8(80, 255 - t);  // fades from 80 → 0

          for (uint16_t i = 0; i < NUM_LEDS; i++) {
            leds[i].r = qadd8(leds[i].r, glow);
            leds[i].g = qadd8(leds[i].g, scale8(glow, 80));
          }
          break;
        }

      // ── ASH: normal fire resumes, no overlay needed ───────────────────────
      case EP_ASH:
      default:
        break;
    }
  }

  // --------------------------------

  void _spawnBurstSparks() {
    for (uint8_t i = 0; i < BURST_SPARK_COUNT; i++) {
      burstSparks[i].alive = true;
      burstSparks[i].x = random8(MATRIX_WIDTH);  // see note below
      burstSparks[i].y = random8(MATRIX_HEIGHT);
      burstSparks[i].brightness = random8(180, 255);
      burstSparks[i].decay = random8(8, 20);
    }
  }

  void _updateBurstSparks() {
    for (uint8_t i = 0; i < BURST_SPARK_COUNT; i++) {
      if (!burstSparks[i].alive) continue;
      if (burstSparks[i].brightness <= burstSparks[i].decay) {
        burstSparks[i].alive = false;
      } else {
        burstSparks[i].brightness -= burstSparks[i].decay;
      }
    }
  }

  void _drawBurstSparks(CRGB* leds) {
    for (uint8_t i = 0; i < BURST_SPARK_COUNT; i++) {
      if (!burstSparks[i].alive) continue;
      uint8_t x = burstSparks[i].x % MATRIX_WIDTH;
      uint8_t y = burstSparks[i].y % MATRIX_HEIGHT;
      uint16_t idx = (uint16_t)y * MATRIX_WIDTH + x;
      uint8_t b = burstSparks[i].brightness;
      leds[idx].r = qadd8(leds[idx].r, b);
      leds[idx].g = qadd8(leds[idx].g, scale8(b, 200));  // warm white
      leds[idx].b = qadd8(leds[idx].b, scale8(b, 120));
    }
  }
  //
};