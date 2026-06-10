#pragma once

#include "config.h"
#include "effects.h"
#include <FastLED.h>

#define AP_STRIP_COUNT 4
#define AP_STRIP_LEDS NUM_LEDS  // Assumed 40 pixels total (indices 0 to 39)

#define FOLLOWER_MIN_BURSTS 3  // Minimum possible bursts on a strip
#define FOLLOWER_MAX_BURSTS 8  // Maximum possible bursts on a strip
#define FOLLOWER_MIN_BURST_DELAY 300
#define FOLLOWER_MAX_BURST_DELAY 750

// Fixed-point traversal speed: Pixels * 256 per second
#define BURST_TRAVERSE_SPEED_FP 5120
#define BURST_WIDTH 5  // Visual width of the pulse heads

// ═════════════════════════════════════════════
// FOLLOWER EFFECT CLASS — Asymmetric Feather Resonance Cascade
// ═════════════════════════════════════════════
class WingsFollowerEffect : public Effect {
public:
  uint32_t phaseStart;

  // Statically allocated 64-byte footprint. Unused slots are disabled via 0xFFFF.
  uint16_t burstOffsets[AP_STRIP_COUNT][FOLLOWER_MAX_BURSTS];
  CRGB burstColor;

  WingsFollowerEffect()
    : Effect(12) {
    Serial.println("Init WingsFollowerEffect (Dynamic Variable Bursts)");
    phaseStart = millis();

    for (uint8_t s = 0; s < AP_STRIP_COUNT; s++) {
      uint32_t runningOffset = random(150);  // Asymmetric initial desync between strips

      // Randomly sample how many bursts this specific strip will execute
      uint8_t activeBurstsCount = random(FOLLOWER_MIN_BURSTS, FOLLOWER_MAX_BURSTS + 1);

      for (uint8_t b = 0; b < FOLLOWER_MAX_BURSTS; b++) {
        if (b < activeBurstsCount) {
          // Set active timing marker
          burstOffsets[s][b] = (uint16_t)runningOffset;

          // Cascade the timeline forward for the next potential burst
          runningOffset += FOLLOWER_MIN_BURST_DELAY + random(FOLLOWER_MAX_BURST_DELAY - FOLLOWER_MIN_BURST_DELAY);
        } else {
          // ── SENTINEL ACTIVATION ──────────────────────────────────────────
          // Disable this slot. 0xFFFF is mathematically unreachable during
          // the effect lifecycle, completely pruning it without taking extra memory.
          burstOffsets[s][b] = 0xFFFF;
        }
      }
    }

    // Radiant phoenix fire profile
    burstColor = CRGB(255, 130, 15);
  }

  // ─────────────────────────────────────────
  void update() override {
    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    bool anyEchoVisible = false;

    // Statelessly calculate if any valid echoes are still processing
    for (uint8_t s = 0; s < AP_STRIP_COUNT; s++) {
      for (uint8_t b = 0; b < FOLLOWER_MAX_BURSTS; b++) {
        uint32_t offset = burstOffsets[s][b];

        // Skip dead sentinel slots immediately
        if (offset == 0xFFFF) continue;

        if (elapsed >= offset) {
          uint32_t activeDuration = elapsed - offset;

          // Echo 2 (Tertiary) position tracking
          uint32_t currentPosFP = (activeDuration * (BURST_TRAVERSE_SPEED_FP - 1024)) / 1000UL;
          uint8_t leadingIndex = currentPosFP >> 8;

          // If the tail of the last echo hasn't cleared the strip, keep the effect alive
          if ((int16_t)leadingIndex - BURST_WIDTH < AP_STRIP_LEDS) {
            anyEchoVisible = true;
          }
        } else {
          // Future scheduled bursts keep the effect alive
          anyEchoVisible = true;
        }
      }
    }

    if (!anyEchoVisible) {
      done = true;
    }
  }

  // ─────────────────────────────────────────
  void draw(CRGB* leds_1, CRGB* leds_2, CRGB* leds_3, CRGB* leds_4) override {
    if (done) return;

    CRGB* stripPtrs[AP_STRIP_COUNT] = { leds_1, leds_2, leds_3, leds_4 };
    uint32_t elapsed = millis() - phaseStart;

    for (uint8_t s = 0; s < AP_STRIP_COUNT; s++) {
      CRGB* strip = stripPtrs[s];
      if (!strip) continue;

      for (uint8_t b = 0; b < FOLLOWER_MAX_BURSTS; b++) {
        uint32_t offset = burstOffsets[s][b];

        // Bypass inactive sentinel slots and unreached timelines
        if (offset == 0xFFFF || elapsed < offset) continue;

        uint32_t activeDuration = elapsed - offset;

        // ── RESONANCE ENGINE (3 Multi-Pass Harmonic Echoes) ────────────────
        for (uint8_t echo = 0; echo < 3; echo++) {
          uint16_t echoSpeed = BURST_TRAVERSE_SPEED_FP;
          uint8_t echoIntensityScale = 255;

          if (echo == 1) {
            echoSpeed = BURST_TRAVERSE_SPEED_FP - 512;
            echoIntensityScale = 128;
          } else if (echo == 2) {
            echoSpeed = BURST_TRAVERSE_SPEED_FP - 1024;
            echoIntensityScale = 51;
          }

          uint32_t currentPosFP = (activeDuration * echoSpeed) / 1000UL;
          uint8_t leadingIndex = currentPosFP >> 8;
          uint8_t fractionalPart = currentPosFP & 0xFF;

          for (uint8_t w = 0; w <= BURST_WIDTH; w++) {
            int16_t pixelIdx = (int16_t)leadingIndex - w;
            if (pixelIdx < 0 || pixelIdx >= AP_STRIP_LEDS) continue;

            uint8_t intensity = 255 - ((w * 255) / BURST_WIDTH);

            if (w == 0) {
              intensity = scale8(intensity, fractionalPart);
            }

            intensity = scale8(intensity, echoIntensityScale);
            if (intensity == 0) continue;

            // Safe additive layering blend across existing fire states
            strip[pixelIdx].r = qadd8(strip[pixelIdx].r, scale8(burstColor.r, intensity));
            strip[pixelIdx].g = qadd8(strip[pixelIdx].g, scale8(burstColor.g, intensity));
            strip[pixelIdx].b = qadd8(strip[pixelIdx].b, scale8(burstColor.b, intensity));
          }
        }
      }
    }
  }
};