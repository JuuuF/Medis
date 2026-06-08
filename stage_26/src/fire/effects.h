#pragma once

#include <FastLED.h>

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

Effect* getEffect(uint8_t effectID);

// ----------------------------------------------

extern Effect *activeEffect;
