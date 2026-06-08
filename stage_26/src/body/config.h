#pragma once

#define DEBUG

#include <FastLED.h>

// ----------------------------------------------
// LED type setup

#ifdef DEBUG
#define CHIPSET WS2812
#define COLOR_ORDER GRB
#else
#define COLOR_ORDER BRG
#define CHIPSET WS2811
#endif  // DEBUG

#define BRIGHTNESS 255

// ----------------------------------------------
// LED matrix setup

#define LED_PIN 6

#ifdef DEBUG
#define MATRIX_HEIGHT 24
#define MATRIX_WIDTH 6
#else
#define MATRIX_HEIGHT 24
#define MATRIX_WIDTH 7
#endif  // DEBUG

#define HEAD_LEDS 4
#define NUM_LEDS (MATRIX_HEIGHT * MATRIX_WIDTH)

// ----------------------------------------------

extern CRGB leds[NUM_LEDS];
