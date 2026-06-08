#include "communication.h"

#include <Arduino.h>

uint8_t receiveEffect() {
  // 1. Wait for start pulse to go low -> true synchronization point
  while (digitalRead(RX_PIN) == HIGH) {}

  // 2. Advance exactly to the dead-center of the first data bit (Bit 7)
  // We skip the remainder of the Start Bit (1000us) and move halfway into Bit 7 (+500us)
  // Total delay from falling edge to center of Bit 7 = 1500us
  delayMicroseconds(BIT_PERIOD_US + (BIT_PERIOD_US / 2));

  uint8_t receivedData = 0;

  // 3. Sample 8 data bits sequentially (MSB to LSB)
  for (uint8_t bit = 0; bit < 8; bit++) {
    // Read the current state of the RX pin
    bool readOne = (digitalRead(RX_PIN) == HIGH);

    // Shift first to open the slot, then append our bit
    receivedData <<= 1;
    if (readOne) {
      receivedData |= 0x01;
    }

    // Only delay if there are more bits left to sample.
    // Stopping on the last bit prevents us from hanging around during the stop bit
    // and causing a false double-read.
    if (bit < 7) {
      delayMicroseconds(BIT_PERIOD_US);
    }
  }

  return receivedData;
}