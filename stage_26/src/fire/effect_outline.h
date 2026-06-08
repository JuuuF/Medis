#pragma once

#include "config.h"
#include "effects.h"
#include <FastLED.h>

class OutlineEffect : public Effect {
public:
  OutlineEffect()
    : Effect(2) {
  }

  void update() override {
  }

  void draw(CRGB *leds) override {
  }
};
