#pragma once

#include <stdint.h>

#include "config.h"

#define TX_PIN 2

// Timing (microseconds)
#define START_PULSE_US 80000      // Long enough to survive a FastLED.show() blackout
#define BIT_PERIOD_US 1000

void broadcastEffect(uint8_t effectID);
