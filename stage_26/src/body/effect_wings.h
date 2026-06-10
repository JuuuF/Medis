#pragma once

#include "config.h"
#include "effects.h"
#include "mapping.h"
#include "communication.h"
#include <FastLED.h>

// ─────────────────────────────────────────────
// Phase Timers
#define MAX_BURSTS 8         // Amount of bursts to show
#define MIN_BURST_DELAY 300  // Minimum delay (ms) between bursts
#define MAX_BURST_DELAY 750  // Maximum delay (ms) between bursts

#define EFFECT_DURATION (MAX_BURSTS * MAX_BURST_DELAY + 2000)

enum EffectPhase : uint8_t {
  EFFECT_PLAYING = 0,
  EFFECT_DONE
};

class WingsEffect : public Effect {
public:
  EffectPhase phase;
  uint32_t phaseStart;

  WingsEffect(uint8_t id)
    : Effect(id) {
    Serial.println("Init WingsEffect");
    phaseStart = millis();
    phase = EFFECT_PLAYING;
    broadcastEffect(effectID);
  }

  void update() override {
    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    switch (phase) {

      case EFFECT_PLAYING:
        {
          if (elapsed >= EFFECT_DURATION) {
            phase = EFFECT_DONE;
            done = true;
          }
          break;
        }

      default: break;
    }
  }

  void draw(CRGB* leds) override {}
};