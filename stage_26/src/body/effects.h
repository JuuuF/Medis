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
  uint8_t effectID;
  bool done = false;

  Effect(uint8_t id) : effectID(id), done(false) {}

  virtual void update() = 0;
  virtual void draw(CRGB *leds) = 0;
  virtual ~Effect() {}
};

// ----------------------------------------------

extern Effect *activeEffect;

void maybeSpawnEffect();
