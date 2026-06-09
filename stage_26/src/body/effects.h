#pragma once

#include <FastLED.h>

// ----------------------------------------------

#ifdef DEBUG
// average seconds between effects
#define EFFECT_INTERVAL_S 2
#else
#define EFFECT_INTERVAL_S 60
#endif

// ----------------------------------------------

class Effect {
public:
  bool done = false;
  bool active = true;
  uint8_t effectID = 0;
  virtual void update() = 0;
  virtual void draw(CRGB *leds) = 0;
  virtual ~Effect() {}

protected:
  explicit Effect(uint8_t id)
    : effectID(id) {}
};

// ----------------------------------------------

extern Effect *activeEffect;

void maybeSpawnEffect();
