#pragma once

#include "config.h"
#include "effects.h"
#include "fire.h"
#include "mapping.h"
#include "communication.h"

// ----------------------------------------------

#define OUTLINE_TRAVEL_MS 4000
#define OUTLINE_HOLD_MS 2000
#define OUTLINE_BEAT_MS 750
#define OUTLINE_DECAY_MS 1500

#define OUTLINE_COLOR_R 180
#define OUTLINE_COLOR_G 80
#define OUTLINE_COLOR_B 255

#define OUTLINE_BG_ALPHA 40
#define OUTLINE_SNAKE_LEN 4
#define OUTLINE_TRAIL_ALPHA 60

#define NECK_HALF (MATRIX_WIDTH / 2)
#define PATH_BODY FLAME_HEIGHT
#define PATH_BOTTOM (MATRIX_WIDTH / 2)  // half-width, drawn symmetrically inward
#define PATH_HEAD HEAD_LEDS
#define PATH_TOP (MATRIX_WIDTH / 2)  // half-width, drawn symmetrically inward
#define PATH_TOTAL (NECK_HALF + PATH_BODY + PATH_BOTTOM + PATH_HEAD + PATH_TOP)

// ----------------------------------------------

class OutlineEffect : public Effect {

  enum Phase { TRAVEL,
               HEAD_CHARGE,
               BEAT,
               DECAY };
  Phase phase = TRAVEL;
  uint32_t phaseStart = 0;
  uint8_t brightness = 255;
  uint16_t tipFP = 0;
  bool beat_triggered = false;

  void advancePhase(Phase next) {
    phase = next;
    phaseStart = millis();
  }

  void addToLED(CRGB *leds, uint8_t col, uint8_t row, uint8_t alpha) {
    if (col >= MATRIX_WIDTH || row >= MATRIX_HEIGHT) return;
    leds[XY(col, row)] += CRGB(
      scale8(OUTLINE_COLOR_R, alpha),
      scale8(OUTLINE_COLOR_G, alpha),
      scale8(OUTLINE_COLOR_B, alpha));
  }

  bool pathToPixel(uint8_t wholePos, uint8_t *col, uint8_t *row, bool *isNeck, bool *isInward) {
    *isNeck = false;
    *isInward = false;

    if (wholePos < NECK_HALF) {
      *isNeck = true;
      *row = FLAME_HEIGHT - 1;
      *col = (MATRIX_WIDTH / 2) - wholePos;
      return true;
    }
    uint8_t pos = wholePos - NECK_HALF;

    if (pos < PATH_BODY) {
      *col = 0;
      *row = (FLAME_HEIGHT - 1) - pos;
      return true;
    }
    pos -= PATH_BODY;

    if (pos < PATH_BOTTOM) {
      *isInward = true;
      *col = pos;
      *row = 0;
      return true;
    }
    pos -= PATH_BOTTOM;

    if (pos < PATH_HEAD) {
      *col = 0;
      *row = FLAME_HEIGHT + pos;
      return true;
    }
    pos -= PATH_HEAD;

    if (pos < PATH_TOP) {
      *isInward = true;
      *col = pos;
      *row = MATRIX_HEIGHT - 1;
      return true;
    }

    return false;
  }

  void drawSymmetric(CRGB *leds, uint8_t col, uint8_t row,
                     bool isNeck, bool isInward, uint8_t alpha) {
    if (isNeck || isInward) {
      // Both neck and inward segments use col as the left position,
      // mirrored to the right
      addToLED(leds, col, row, alpha);
      addToLED(leds, (MATRIX_WIDTH - 1) - col, row, alpha);
    } else {
      // Side segments: always left and right edges
      addToLED(leds, 0, row, alpha);
      addToLED(leds, MATRIX_WIDTH - 1, row, alpha);
    }
  }

  void drawHorizontalLine(CRGB *leds, uint8_t row, uint8_t alpha) {
    for (uint8_t col = 0; col < MATRIX_WIDTH; col++) {
      addToLED(leds, col, row, alpha);
    }
  }

  void drawSnake(CRGB *leds, uint8_t alpha) {
    uint8_t tipWhole = tipFP >> 8;
    uint8_t tipFrac = tipFP & 0xFF;

    {
      uint8_t col, row;
      bool isNeck, isInward;
      if (pathToPixel(tipWhole + 1, &col, &row, &isNeck, &isInward))
        drawSymmetric(leds, col, row, isNeck, isInward, scale8(alpha, tipFrac));
    }
    {
      uint8_t col, row;
      bool isNeck, isInward;
      if (pathToPixel(tipWhole, &col, &row, &isNeck, &isInward))
        drawSymmetric(leds, col, row, isNeck, isInward, alpha);
    }
    for (uint8_t j = 1; j <= OUTLINE_SNAKE_LEN; j++) {
      if (j > tipWhole) break;
      uint8_t col, row;
      bool isNeck, isInward;
      if (!pathToPixel(tipWhole - j, &col, &row, &isNeck, &isInward)) break;
      uint8_t trailAlpha = alpha - (j * (alpha / (OUTLINE_SNAKE_LEN + 1)));
      if (j == OUTLINE_SNAKE_LEN) trailAlpha = scale8(trailAlpha, 255 - tipFrac);
      drawSymmetric(leds, col, row, isNeck, isInward, trailAlpha);
    }
  }

  void drawTrail(CRGB *leds, uint8_t alpha) {
    uint8_t tipWhole = tipFP >> 8;
    uint8_t end = tipWhole > OUTLINE_SNAKE_LEN ? tipWhole - OUTLINE_SNAKE_LEN : 0;
    for (uint8_t i = 0; i < end; i++) {
      uint8_t col, row;
      bool isNeck, isInward;
      if (pathToPixel(i, &col, &row, &isNeck, &isInward))
        drawSymmetric(leds, col, row, isNeck, isInward, alpha);
    }
  }

  void drawFullOutline(CRGB *leds, uint8_t alpha) {
    // Neck
    drawHorizontalLine(leds, FLAME_HEIGHT - 1, alpha);

    // Body sides
    for (uint8_t row = 0; row < FLAME_HEIGHT - 1; row++) {
      addToLED(leds, 0, row, alpha);
      addToLED(leds, MATRIX_WIDTH - 1, row, alpha);
    }

    // Body bottom
    drawHorizontalLine(leds, 0, alpha);

    // Head sides
    for (uint8_t row = FLAME_HEIGHT + 1; row < MATRIX_HEIGHT; row++) {
      addToLED(leds, 0, row, alpha);
      addToLED(leds, MATRIX_WIDTH - 1, row, alpha);
    }

    // Head top
    drawHorizontalLine(leds, MATRIX_HEIGHT - 1, alpha);
  }


public:
  OutlineEffect(uint8_t id)
    : Effect(id) {
    Serial.println("Init Outline Effect");
    phaseStart = millis();
  }

  void update() override {
    uint32_t elapsed = millis() - phaseStart;

    switch (phase) {
      case TRAVEL:
        {
          uint32_t fullFP = (uint32_t)PATH_TOTAL << 8;
          tipFP = (uint16_t)min((elapsed * fullFP) / OUTLINE_TRAVEL_MS, fullFP);

          if (elapsed >= OUTLINE_TRAVEL_MS)
            advancePhase(HEAD_CHARGE);

          break;
        }
      case HEAD_CHARGE:
        {
          // charge ramps up over time (0 → 255)
          uint8_t charge = (uint8_t)min((elapsed * 255UL) / OUTLINE_HOLD_MS, 255UL);

          // use charge to drive head “energy” (stored in brightness for now)
          brightness = scale8(charge, charge);  // accelerating curve

          if (elapsed >= OUTLINE_HOLD_MS)
            advancePhase(BEAT);

          break;
        }
      case BEAT:
        {
          uint8_t t = (uint8_t)min((elapsed * 255UL) / OUTLINE_BEAT_MS, 255UL);

          // sharp release from charged state
          brightness = 255 - scale8(t, t);

          // output to other Arduinos
          if (!beat_triggered && brightness > 250) {
            beat_triggered = true;
            broadcastEffect(effectID);
          }

          if (elapsed >= OUTLINE_BEAT_MS) {
            advancePhase(DECAY);
          }

          break;
        }
      case DECAY:
        brightness = 255 - (uint8_t)min((elapsed * 255UL) / OUTLINE_DECAY_MS, 255UL);
        if (elapsed >= OUTLINE_DECAY_MS) done = true;
        break;
    }
  }

  void draw(CRGB *leds) override {
    switch (phase) {
      case TRAVEL:
        drawTrail(leds, OUTLINE_TRAIL_ALPHA);
        drawSnake(leds, 255);
        break;
      case HEAD_CHARGE:
        {
          // body stays stable
          drawFullOutline(leds, OUTLINE_BG_ALPHA);

          // head “comes alive” (simple modulation)
          uint8_t headAlpha = scale8(brightness, 180);

          for (uint8_t row = FLAME_HEIGHT; row < MATRIX_HEIGHT; row++) {
            addToLED(leds, 0, row, headAlpha);
            addToLED(leds, MATRIX_WIDTH - 1, row, headAlpha);
          }

          break;
        }
      case BEAT:
        drawFullOutline(leds, brightness);
        break;
      case DECAY:
        drawFullOutline(leds, scale8(brightness, OUTLINE_BG_ALPHA));
        break;
    }
  }
};
