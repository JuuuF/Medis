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

  void draw(CRGB *leds_1, CRGB *leds_2, CRGB *leds_3, CRGB *leds_4) override {
  }
};
