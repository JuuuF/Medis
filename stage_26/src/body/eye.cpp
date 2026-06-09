#include "eye.h"

void renderIndependentEye(CRGB* leds) {
  uint32_t now = millis();

  // Keep track of the timeline tracking states across frames
  static uint32_t nextBlinkTime = 0;
  static uint32_t currentWaitDuration = 3000;

  // Initialize the first tracking point on boot up
  if (nextBlinkTime == 0) {
    nextBlinkTime = now + currentWaitDuration;
  }

  uint8_t rawBrightness = 255;  // Default: wide open

  // Check if we are currently within a blinking window
  if (now >= nextBlinkTime) {
    uint32_t blinkElapsed = now - nextBlinkTime;

    if (blinkElapsed < EYE_BLINK_DURATION_MS) {
      // Scale down current blink window time to a 0-255 phase space
      uint8_t blinkPhase = (blinkElapsed * 255) / EYE_BLINK_DURATION_MS;

      // V-shape dip down and snap back up
      uint8_t angle = map8(blinkPhase, 64, 192);
      rawBrightness = sin8(angle);
    } else {
      // The blink cycle has fully concluded!
      // 1. Roll a new randomized wait delay time
      currentWaitDuration = random16(EYE_MIN_WAIT_MS, EYE_MAX_WAIT_MS);

      // 2. Schedule the timestamp for when the next blink will execute
      nextBlinkTime = now + currentWaitDuration;
    }
  } else {
    // Subtle organic micro-tremble while awake so the eye looks living
    rawBrightness = beatsin8(7, 242, 255);
  }

  // Enforce the brightness floor so it never goes dark
  uint8_t targetBrightness = map8(rawBrightness, EYE_MIN_BRIGHTNESS, EYE_MAX_BRIGHTNESS);

  // Generate color and display to frame
  CHSV eyeHsv(FC_EYE_HUE, FC_EYE_SAT, targetBrightness);
  CRGB eyeColor;
  hsv2rgb_rainbow(eyeHsv, eyeColor);

  fill_solid(leds + MATRIX_EYE_INDEX, EYE_LEDS, eyeColor);
}