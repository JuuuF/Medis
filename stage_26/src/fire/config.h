#include <FastLED.h>

// ----------------------------------------------
// Strip settings

#define LED_PIN_1 3
#define LED_PIN_2 6
#define LED_PIN_3 9
#define LED_PIN_4 12

#define DEBUG

#ifdef DEBUG
#define COLOR_ORDER GRB
#define CHIPSET WS2812
#else
#define COLOR_ORDER BRG
#define CHIPSET WS2811
#endif

#define NUM_LEDS 40
#define BRIGHTNESS 255

// ----------------------------------------------

extern CRGB leds[NUM_LEDS];
