#include "communication.h"


void broadcastEffect(uint8_t effectID) {
  noInterrupts();

  // Start pulse
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(START_PULSE_US);
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(BIT_PERIOD_US);  // idle gap

  // 8 data bits, MSB first
  for (int8_t bit = 7; bit >= 0; bit--) {
    bool isOne = (effectID >> bit) & 0x01;
    uint16_t highTime = isOne ? BIT_ONE_HIGH_US : BIT_ZERO_HIGH_US;

    digitalWrite(TX_PIN, HIGH);
    delayMicroseconds(highTime);
    digitalWrite(TX_PIN, LOW);
    delayMicroseconds(BIT_PERIOD_US - highTime);
  }

  interrupts();
}
