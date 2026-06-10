#pragma once

#include "config.h"

#include <FastLED.h>

#ifdef DEBUG
#define MATRIX_EYE_INDEX 49
#define EYE_LEDS 1
#else
#define MATRIX_EYE_INDEX 120
#define EYE_LEDS 1
#define EYE_PIN 13
#endif  // DEBUG

#define FC_EYE_HUE 0  // Crimson / Deep Red (or match amber-gold with 30)
#define FC_EYE_SAT 0
#define FC_EYE_PULSE_MS 3000  // Total duration of one breathing cycle

/// Custom adjustments for the natural blink feel
#define EYE_MIN_BRIGHTNESS 60      // The floor value so the eye is never fully off
#define EYE_MAX_BRIGHTNESS 255     // Peak brightness
#define EYE_BLINK_DURATION_MS 750  // Sharp, fast eye-blink transition

// Randomized timing limits (in milliseconds)
#define EYE_MIN_WAIT_MS 5000   // Minimum time between blinks (1.5 seconds)
#define EYE_MAX_WAIT_MS 10000  // Maximum time between blinks (6 seconds)


/**
 * Renders an isolated, independent animation onto the designated Eye LED.
 * Call this at the absolute end of your simulation step, right before FastLED.show().
 */
void renderIndependentEye(CRGB* leds);
