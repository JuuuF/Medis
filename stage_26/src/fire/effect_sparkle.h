#pragma once

#include "config.h"
#include "effects.h"
#include <FastLED.h>

class SparkleEffect : public Effect {
public:
  SparkleEffect()
    : Effect(1) {
  }

  void update() override {
  }

  void draw(CRGB *leds) override {
  }
};
