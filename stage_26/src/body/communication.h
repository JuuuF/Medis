#include <stdint.h>

#include "config.h"

#define TX_PIN 2

// Timing (microseconds)
#define START_PULSE_US 3000      // Long enough to survive a FastLED.show() blackout
#define START_PULSE_MIN_US 2000  // Minimum valid start pulse (noise rejection)
#define BIT_PERIOD_US 200
#define BIT_ONE_HIGH_US 140
#define BIT_ZERO_HIGH_US 60
#define BIT_SAMPLE_US 100  // sample halfway between '0' and '1' widths

void broadcastEffect(uint8_t effectID);
