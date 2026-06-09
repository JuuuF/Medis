#include "Arduino.h"
#include "communication.h"


void broadcastEffect(uint8_t effectID) {
  noInterrupts();

  // 1. Long high wake-up pulse (to clear out any FastLED blackout)
  digitalWrite(TX_PIN, HIGH);
  delayMicroseconds(START_PULSE_US);  // 3000us

  // 2. Start Bit (Falling Edge) - Held for 200us
  digitalWrite(TX_PIN, LOW);
  delayMicroseconds(BIT_PERIOD_US);

  // 3. Transmit 8 Bits (MSB first), Time-Based
  for (int8_t bit = 7; bit >= 0; bit--) {
    bool isOne = (effectID >> bit) & 0x01;

    digitalWrite(TX_PIN, isOne ? HIGH : LOW);
    delayMicroseconds(BIT_PERIOD_US);
  }

  // 4. Return line to HIGH (Stop bit)
  digitalWrite(TX_PIN, LOW);

  interrupts();
  Serial.print("Broadcast effectID: ");
  Serial.println(effectID);
}
