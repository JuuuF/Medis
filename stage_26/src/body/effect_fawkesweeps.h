#pragma once

#include "config.h"
#include "effects.h"
#include "mapping.h"
#include "communication.h"
#include <FastLED.h>

// ─────────────────────────────────────────────
// Phase Durations
#define FW_MOURN_MS 10000  // Cold grief desaturation builds from crown down
#define FW_TEARS_MS 4000   // Tear particles fall; broadcast fires on first tear
#define FW_FLASH_MS 300    // White resurrection detonation
#define FW_BLOOM_MS 5000   // Warm golden afterglow decays to normal fire

// ─────────────────────────────────────────────
// Mourn-phase colour constants
// The fire is gradually pushed toward a cold blue-violet grief palette.
// We sap green and boost blue, scaling by the mourn ramp.
#define FW_MOURN_GREEN_SAP 200  // scale8 keepFactor for green (lower = more sapped)
#define FW_MOURN_BLUE_BOOST 60  // peak additive blue per pixel at full mourn
#define FW_MOURN_RED_SAP 210    // keepFactor for red at full mourn depth

// ─────────────────────────────────────────────
// Tear particle constants
#define FW_TEAR_COUNT 4       // Total tear particles spawned across FW_TEARS_MS
#define FW_TEAR_SPEED_FP 310  // Fixed-point pixels/sec * 64 — falls 40px in ~800ms
#define FW_TEAR_HEAD_LEN 5    // Anti-aliased glow window ahead of tear core
#define FW_TEAR_TRAIL_LEN 6   // Cooling trail length behind the tear core
// Tear colour: bright blue-white core fading to cold silver-violet trail
// Core  : R=160  G=200  B=255  (cool blue-white)
// Trail : R= 60  G= 80  B=200  (cold violet)
// Deep trail: R=20  G=20  B=100

// Stagger: tears spawn at evenly spaced intervals across FW_TEARS_MS
// Interval = FW_TEARS_MS / FW_TEAR_COUNT

// ─────────────────────────────────────────────
// Flash-phase constants
#define FW_FLASH_CX (MATRIX_WIDTH / 2)   // Bell-curve expansion origin X
#define FW_FLASH_CY (MATRIX_HEIGHT / 2)  // Bell-curve expansion origin Y
// Flash radius grows from 0 to diagonal; pixels within radius get full white

// ─────────────────────────────────────────────
// Bloom constants
#define FW_BLOOM_RED 220    // Peak additive red in the golden bloom
#define FW_BLOOM_GREEN 100  // Peak additive green in the golden bloom

// ─────────────────────────────────────────────
// Phase enum
enum FWPhase : uint8_t {
  FW_MOURN = 0,
  FW_TEARS,
  FW_FLASH,
  FW_BLOOM,
  FW_DONE
};

// ─────────────────────────────────────────────
// Tear particle state
struct FWTear {
  bool active;     // Is this slot live?
  bool spawned;    // Has spawn time been reached yet?
  uint16_t posFP;  // Fixed-point position: 64ths of a pixel, origin = FULL_HEIGHT top
  // posFP = 0 means the crown row (MATRIX_HEIGHT - 1)
  // posFP = (MATRIX_HEIGHT - 1) * 64 means the floor (row 0)
};

// ═════════════════════════════════════════════
// LEADER EFFECT CLASS  —  Effect ID 8
// ═════════════════════════════════════════════
class FawkesWeepsEffect : public Effect {
public:
  FWPhase phase;
  uint32_t phaseStart;

  // Mourn ramp: 0–255 over FW_MOURN_MS, cached each update() tick
  uint8_t mournRamp;

  // Broadcast guard
  bool broadcastFired;

  // Tear particle pool
  FWTear tears[FW_TEAR_COUNT];

  // Flash alpha: full at phase entry, decays to 0
  uint8_t flashAlpha;

  // Bloom alpha: full at phase entry, lerps to 0
  uint8_t bloomAlpha;

  // ─────────────────────────────────────────────
  FawkesWeepsEffect(uint8_t id)
    : Effect(id) {
    Serial.println("Init Fawke Weeps Effect");
    phaseStart = millis();
    phase = FW_MOURN;
    mournRamp = 0;
    broadcastFired = false;
    flashAlpha = 255;
    bloomAlpha = 255;

    // Pre-initialise all tears as unspawned
    for (uint8_t i = 0; i < FW_TEAR_COUNT; i++) {
      tears[i].active = false;
      tears[i].spawned = false;
      tears[i].posFP = 0;
    }
  }

  // ─────────────────────────────────────────────
  void update() override {
    uint32_t now = millis();
    uint32_t elapsed = now - phaseStart;

    switch (phase) {

      // ── MOURN: build the cold desaturation mask over 5 seconds ──────────
      case FW_MOURN:
        mournRamp = (elapsed >= FW_MOURN_MS)
                      ? 255
                      : (uint8_t)((elapsed * 255UL) / FW_MOURN_MS);
        if (elapsed >= FW_MOURN_MS) {
          phase = FW_TEARS;
          phaseStart = now;
        }
        break;

      // ── TEARS: advance particles, spawn sequentially ─────────────────────
      case FW_TEARS:
        {
          // Spawn interval: evenly distribute tears across FW_TEARS_MS
          // Tear i spawns at elapsed >= i * (FW_TEARS_MS / FW_TEAR_COUNT)
          const uint16_t spawnInterval = FW_TEARS_MS / FW_TEAR_COUNT;  // 1000ms

          for (uint8_t i = 0; i < FW_TEAR_COUNT; i++) {
            uint32_t spawnTime = (uint32_t)i * spawnInterval;

            // Spawn guard: activate tear on its window
            if (!tears[i].spawned && elapsed >= spawnTime) {
              tears[i].spawned = true;
              tears[i].active = true;
              tears[i].posFP = 0;  // Start at crown

              // Broadcast on first tear spawn (tear 0)
              if (i == 0 && !broadcastFired) {
                broadcastEffect(effectID);
                broadcastFired = true;
              }
            }

            // Advance active tears
            if (tears[i].active) {
              // 8ms tick: advance by FW_TEAR_SPEED_FP / 8  (>>3)
              tears[i].posFP += (FW_TEAR_SPEED_FP >> 3);

              // Maximum travel distance: full matrix height in FP units
              uint16_t maxFP = (uint16_t)(MATRIX_HEIGHT - 1) * 64;
              if (tears[i].posFP >= maxFP) {
                tears[i].posFP = maxFP;
                tears[i].active = false;  // Tear has hit the floor
              }
            }
          }

          // Transition when all tears have fallen AND time window is over
          bool allFallen = true;
          for (uint8_t i = 0; i < FW_TEAR_COUNT; i++) {
            if (tears[i].active) {
              allFallen = false;
              break;
            }
          }
          if (allFallen && elapsed >= FW_TEARS_MS) {
            phase = FW_FLASH;
            phaseStart = now;
            flashAlpha = 255;
          }
          break;
        }

      // ── FLASH: rapid bell-curve detonation ──────────────────────────────
      case FW_FLASH:
        flashAlpha = (elapsed >= FW_FLASH_MS)
                       ? 0
                       : (uint8_t)(255 - (elapsed * 255UL) / FW_FLASH_MS);
        if (elapsed >= FW_FLASH_MS) {
          phase = FW_BLOOM;
          phaseStart = now;
          bloomAlpha = 255;
        }
        break;

      // ── BLOOM: warm golden afterglow decays to zero ──────────────────────
      case FW_BLOOM:
        bloomAlpha = (elapsed >= FW_BLOOM_MS)
                       ? 0
                       : lerp8by8(255, 0, (uint8_t)((elapsed * 255UL) / FW_BLOOM_MS));
        if (elapsed >= FW_BLOOM_MS) {
          phase = FW_DONE;
          done = true;
        }
        break;

      default:
        break;
    }
  }

  // ─────────────────────────────────────────────
  void draw(CRGB* leds) override {

    switch (phase) {

      // ════════════════════════════════════════
      // MOURN
      // Crown-down cold shift: green sapped, blue boosted.
      // The effect grows downward from the head as mournRamp rises.
      // Spatial mask: a pixel at row y receives suppression scaled by
      //   how far it is from the crown and how far mournRamp has progressed.
      //
      // Concretely: coldDepth is the number of rows from the crown that
      // are currently "infected" by grief, advancing as mournRamp grows.
      // Within that zone, suppression is strongest near the crown and
      // tapers off at the advancing wavefront.
      // ════════════════════════════════════════
      case FW_MOURN:
        {
          // Total rows including head region
          uint8_t totalRows = MATRIX_HEIGHT;  // crown = MATRIX_HEIGHT-1, floor = 0

          // How many rows from crown are currently in the grief zone?
          // At mournRamp=0: 0 rows. At mournRamp=255: all rows.
          uint8_t coldRows = (uint8_t)((uint16_t)totalRows * mournRamp / 255);

          for (uint8_t y = 0; y < totalRows; y++) {
            // y=0 = floor, y=MATRIX_HEIGHT-1 = crown.
            // Distance from crown: (MATRIX_HEIGHT - 1 - y)
            uint8_t distFromCrown = (MATRIX_HEIGHT - 1) - y;

            if (distFromCrown >= coldRows) continue;  // Not yet in grief zone

            // Intensity ramps: strongest at crown, fades at wavefront
            // suppressStrength = 255 at crown, 0 at wavefront edge
            uint8_t suppressStrength = (coldRows > 0)
                                         ? (uint8_t)(255UL * (coldRows - distFromCrown) / coldRows)
                                         : 0;

            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
              uint16_t idx = XY(x, y);

              // Sap red slightly — deep cold shift
              leds[idx].r = scale8(leds[idx].r,
                                   lerp8by8(255, FW_MOURN_RED_SAP, suppressStrength));

              // Heavily drain green — removes warmth
              leds[idx].g = scale8(leds[idx].g,
                                   lerp8by8(255, FW_MOURN_GREEN_SAP, suppressStrength));

              // Boost blue — inject cold grief tone
              uint8_t blueAdd = scale8(FW_MOURN_BLUE_BOOST, suppressStrength);
              leds[idx].b = qadd8(leds[idx].b, blueAdd);
            }
          }

          // ── Head pulse: faint blue-white breathing at the crown ──────────
          // Uses mournRamp as a slow brightener so the head noticeably
          // leads the grief effect downward.
          for (uint8_t y = FLAME_HEIGHT; y < MATRIX_HEIGHT; y++) {
            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
              uint16_t idx = XY(x, y);
              uint8_t headGlow = scale8(40, mournRamp);
              leds[idx].b = qadd8(leds[idx].b, headGlow);
              leds[idx].g = qadd8(leds[idx].g, scale8(headGlow, 160));
            }
          }
          break;
        }

      // ════════════════════════════════════════
      // TEARS
      // Full grief suppression is locked at mournRamp=255 throughout.
      // Tear particles rendered as anti-aliased streaks falling from crown.
      // The grief colour layer still applies, then tears are additive on top.
      // ════════════════════════════════════════
      case FW_TEARS:
        {
          // ── Lock grief mask at full strength across entire matrix ──────────
          for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
              uint16_t idx = XY(x, y);
              leds[idx].r = scale8(leds[idx].r, FW_MOURN_RED_SAP);
              leds[idx].g = scale8(leds[idx].g, FW_MOURN_GREEN_SAP);
              leds[idx].b = qadd8(leds[idx].b, FW_MOURN_BLUE_BOOST >> 1);
            }
          }

          // ── Render each active tear ──────────────────────────────────────
          for (uint8_t i = 0; i < FW_TEAR_COUNT; i++) {
            if (!tears[i].spawned) continue;

            // Integer row of tear core; origin is crown = (MATRIX_HEIGHT-1)
            // posFP is distance fallen from crown; convert to absolute row:
            //   row = (MATRIX_HEIGHT - 1) - (posFP >> 6)
            uint8_t fallen = (uint8_t)(tears[i].posFP >> 6);
            int8_t coreRow = (int8_t)(MATRIX_HEIGHT - 1) - (int8_t)fallen;

            // Sub-pixel fractional blend (0–63 → 0–255 for alpha)
            uint8_t frac = (uint8_t)((tears[i].posFP & 0x3F) << 2);

            // Tears fall in columns — each tear uses its own column
            // Distribute evenly across the matrix width for visual spread
            uint8_t col = (uint8_t)((uint16_t)i * MATRIX_WIDTH / FW_TEAR_COUNT)
                          + (MATRIX_WIDTH / (FW_TEAR_COUNT * 2));
            // clamp
            if (col >= MATRIX_WIDTH) col = MATRIX_WIDTH - 1;

            // ── Core pixel: bright blue-white ────────────────────────────
            _paintTear(leds, col, coreRow, 160, 200, 255, 255);
            // ── Sub-pixel anti-alias: fractional pixel ahead of core ─────
            _paintTear(leds, col, coreRow + 1, 160, 200, 255, 255 - frac);

            // ── Trail pixels cooling behind the core ─────────────────────
            // -1: still bright, slight cool
            _paintTear(leds, col, coreRow - 1, 100, 140, 255, 220);
            // -2: mid violet
            _paintTear(leds, col, coreRow - 2, 60, 80, 200, 170);
            // -3: cooling violet-blue
            _paintTear(leds, col, coreRow - 3, 30, 40, 150, 110);
            // -4: deep cold residue
            _paintTear(leds, col, coreRow - 4, 15, 20, 100, 70);
            // -5: faint cold whisper
            _paintTear(leds, col, coreRow - 5, 5, 8, 50, 40);
          }
          break;
        }

      // ════════════════════════════════════════
      // FLASH
      // Pure-white bell-curve radial expansion from matrix center.
      // All pixels within the expanding radius saturate to white,
      // scaled by flashAlpha so the whole thing fades as it expands.
      // ════════════════════════════════════════
      case FW_FLASH:
        {
          // Radius grows from 0 → max diagonal over FW_FLASH_MS
          // Using elapsed from phase start (tracked via flashAlpha decay)
          // flashAlpha: 255 → 0; invert to get progress 0 → 255
          uint8_t progress = 255 - flashAlpha;

          // Max radius: ceil of diagonal in fixed-point (* 4 for precision)
          // Diagonal ≈ sqrt(W² + H²); we approximate with W + H (safe upper bound)
          uint8_t maxRad = MATRIX_WIDTH + MATRIX_HEIGHT;

          // Current radius (integer pixels)
          uint8_t currentRad = (uint8_t)((uint16_t)maxRad * progress / 255);

          for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
              // Manhattan distance from center (fast, no sqrt)
              uint8_t dx = (x > FW_FLASH_CX) ? (x - FW_FLASH_CX) : (FW_FLASH_CX - x);
              uint8_t dy = (y > FW_FLASH_CY) ? (y - FW_FLASH_CY) : (FW_FLASH_CY - y);
              uint8_t dist = dx + dy;

              if (dist > currentRad) continue;  // Outside blast radius

              // Intensity: full white at core, slight falloff at radius edge
              uint8_t edgeFade = (currentRad > 0)
                                   ? (uint8_t)(255UL * (currentRad - dist) / currentRad)
                                   : 255;
              uint8_t brightness = scale8(edgeFade, flashAlpha);
              // flashAlpha decreasing → global dim as it expands

              uint16_t idx = XY(x, y);
              leds[idx].r = qadd8(leds[idx].r, brightness);
              leds[idx].g = qadd8(leds[idx].g, brightness);
              leds[idx].b = qadd8(leds[idx].b, brightness);
            }
          }
          break;
        }

      // ════════════════════════════════════════
      // BLOOM
      // Warm golden additive layer decays as bloomAlpha lerps to 0.
      // Stronger in the upper body / head (fire is "reigniting" from top).
      // No suppression — native fire returns unobstructed.
      // ════════════════════════════════════════
      case FW_BLOOM:
        {
          if (bloomAlpha == 0) break;

          for (uint8_t y = 0; y < MATRIX_HEIGHT; y++) {
            // Vertical taper: bloom is brighter near crown, dimmer at floor
            // representing the phoenix's fire surging back to life top-first
            uint8_t vertFrac = (uint8_t)(255UL * y / (MATRIX_HEIGHT - 1));
            uint8_t bloomLocal = scale8(bloomAlpha, lerp8by8(80, 255, vertFrac));

            for (uint8_t x = 0; x < MATRIX_WIDTH; x++) {
              uint16_t idx = XY(x, y);
              leds[idx].r = qadd8(leds[idx].r, scale8(FW_BLOOM_RED, bloomLocal));
              leds[idx].g = qadd8(leds[idx].g, scale8(FW_BLOOM_GREEN, bloomLocal));
              // no blue — purges the grief palette and restores warmth
            }
          }
          break;
        }

      default:
        break;
    }
  }

private:
  // ── Bounds-safe additive tear pixel painter ───────────────────────────────
  // Accepts signed row (tear can overshoot during sub-pixel calculation).
  inline void _paintTear(CRGB* leds, uint8_t x, int8_t y,
                         uint8_t r, uint8_t g, uint8_t b, uint8_t alpha) {
    if (y < 0 || y >= (int8_t)MATRIX_HEIGHT) return;
    uint16_t idx = XY(x, (uint8_t)y);
    leds[idx].r = qadd8(leds[idx].r, scale8(r, alpha));
    leds[idx].g = qadd8(leds[idx].g, scale8(g, alpha));
    leds[idx].b = qadd8(leds[idx].b, scale8(b, alpha));
  }

  void _blend(CRGB* leds, int8_t x, int8_t y, CRGB col) {
    if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return;
    uint16_t idx = XY(x, y);
    leds[idx].r = qadd8(leds[idx].r, col.r);
    leds[idx].g = qadd8(leds[idx].g, col.g);
    leds[idx].b = qadd8(leds[idx].b, col.b);
  }
};
