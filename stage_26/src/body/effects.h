#pragma once

#include <FastLED.h>

// ----------------------------------------------

#define EFFECT_INTERVAL_S 10  // average seconds between effects

// ----------------------------------------------

class Effect {
public:
  bool done = false;
  bool active = true;
  virtual void update() = 0;
  virtual void draw(CRGB *leds) = 0;
  virtual ~Effect() {}
};

// ----------------------------------------------

extern Effect *activeEffect;

void maybeSpawnEffect();
