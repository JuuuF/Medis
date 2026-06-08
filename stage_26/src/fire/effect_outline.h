#pragma once

#include "config.h"
#include "effects.h"
#include <FastLED.h>

// ----------------------------------------------

#define OUTLINE_COLOR_R 180
#define OUTLINE_COLOR_G 80
#define OUTLINE_COLOR_B 255

#define OUTLINE_SNAKE_LEN 6
#define OUTLINE_FADE_ZONE 10

// ----------------------------------------------

uint16_t getSkewedSpeed() {
  // 1. Get a uniform random number from 0 to 255
  uint32_t r = random8();

  // 2. Square it (r * r). This stretches 0-255 into a range of 0-65025.
  // Because it's squared, low numbers stay very low, and high numbers shoot up fast.
  uint32_t skewed = r * r;

  // 3. Map that skewed value from 0-65025 into your target speed range (e.g., 200 to 800)
  uint16_t minSpeed = 50;
  uint16_t maxSpeed = 600;

  return minSpeed + ((skewed * (maxSpeed - minSpeed)) / 65025UL);
}

class OutlineEffect : public Effect {
private:
  uint32_t startTime;
  uint16_t tipFP[4];
  uint16_t speedFP[4];
  bool allStripsFinished = false;

  // Helper macro/function to calculate linear edge decay per individual pixel
  inline uint8_t getPixelFade(int16_t pixelIdx) {
#if defined(OUTLINE_FADE_ZONE) && OUTLINE_FADE_ZONE > 0
    const int16_t fadeStart = NUM_LEDS - OUTLINE_FADE_ZONE;
    if (pixelIdx >= fadeStart) {
      int16_t remaining = NUM_LEDS - 1 - pixelIdx;
      // Linearly scale from 255 (at fadeStart) down to 0 (at the absolute last pixel)
      return (remaining * 255) / OUTLINE_FADE_ZONE;
    }
#endif
    return 255;  // Default full brightness outside the zone
  }

public:
  OutlineEffect()
    : Effect(2) {
    Serial.println("Outline effect");
    startTime = millis();

    for (uint8_t i = 0; i < 4; i++) {
      tipFP[i] = 0;
      speedFP[i] = getSkewedSpeed();
    }
  }

  void update() override {
    if (done) return;

    allStripsFinished = true;
    // We let the simulation run slightly past the end so the tail fully clears out
    uint32_t targetMaxPositionFP = (uint32_t)(NUM_LEDS + OUTLINE_SNAKE_LEN) << 8;

    for (uint8_t i = 0; i < 4; i++) {
      if (tipFP[i] < targetMaxPositionFP) {
        tipFP[i] += speedFP[i];
        allStripsFinished = false;
      }
    }

    if (allStripsFinished) {
      done = true;
    }
  }

  void draw(CRGB *leds_1, CRGB *leds_2, CRGB *leds_3, CRGB *leds_4) override {
    CRGB *stripPointers[4] = { leds_1, leds_2, leds_3, leds_4 };

    for (uint8_t s = 0; s < 4; s++) {
      CRGB *currentStrip = stripPointers[s];
      uint8_t tipWhole = tipFP[s] >> 8;
      uint8_t tipFrac = tipFP[s] & 0xFF;

      if (tipWhole >= NUM_LEDS + OUTLINE_SNAKE_LEN) continue;

      // 1. Draw the anti-aliased leading pixel tip
      if (tipWhole < NUM_LEDS) {
        uint8_t finalAlpha = scale8(tipFrac, getPixelFade(tipWhole));
        currentStrip[tipWhole] += CRGB(
          scale8(OUTLINE_COLOR_R, finalAlpha),
          scale8(OUTLINE_COLOR_G, finalAlpha),
          scale8(OUTLINE_COLOR_B, finalAlpha));
      }

      // 2. Draw the main head pixel
      if (tipWhole > 0 && (tipWhole - 1) < NUM_LEDS) {
        int16_t headIdx = tipWhole - 1;
        uint8_t finalAlpha = getPixelFade(headIdx);
        currentStrip[headIdx] += CRGB(
          scale8(OUTLINE_COLOR_R, finalAlpha),
          scale8(OUTLINE_COLOR_G, finalAlpha),
          scale8(OUTLINE_COLOR_B, finalAlpha));
      }

      // 3. Render the fading trail trailing behind the tip
      for (uint8_t j = 2; j <= OUTLINE_SNAKE_LEN; j++) {
        if (j > tipWhole) break;

        int16_t pixelIdx = tipWhole - j;
        if (pixelIdx >= NUM_LEDS) continue;

        // Base structural fade of the tail gradient
        uint8_t trailAlpha = 255 - (j * (255 / (OUTLINE_SNAKE_LEN + 1)));

        // Multiplied by its unique position-based edge fade
        uint8_t finalAlpha = scale8(trailAlpha, getPixelFade(pixelIdx));

        currentStrip[pixelIdx] += CRGB(
          scale8(OUTLINE_COLOR_R, finalAlpha),
          scale8(OUTLINE_COLOR_G, finalAlpha),
          scale8(OUTLINE_COLOR_B, finalAlpha));
      }
    }
  }
};
