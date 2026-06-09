#include "effects.h"

#include "config.h"
#include "mapping.h"

#include "effect_sparkle.h"
#include "effect_outline.h"
#include "effect_ember.h"
#include "effect_rebirth.h"
#include "effect_feather.h"

#include <FastLED.h>

// ----------------------------------------------

struct EffectEntry {
  Effect* (*create)();
  uint8_t weight;
};

// Add effects here
const EffectEntry effects[] = {
  { []() { return new SparkleEffect(); }, 2},
  { []() { return new OutlineEffect(); }, 2},
  { []() { return new EmberEffect(); }, 2},
  { []() { return new RebirthEffect(); }, 2},
  { []() { return new FeatherCascadeEffect(); }, 2},
};

Effect* pickEffect() {
  // Sum up total weight
  uint16_t total = 0;
  for (auto& e : effects) total += e.weight;

  uint16_t pick = random(total);
  uint16_t cumulative = 0;
  for (auto& e : effects) {
    cumulative += e.weight;
    if (pick < cumulative) {
      return e.create();
    }
  }

  // Base case: effect 0 (should never be reached!)
  return effects[0].create();
}

void maybeSpawnEffect() {
  if (EFFECT_INTERVAL_S == 0) return;
  if (activeEffect != nullptr) return;

  static uint32_t nextSpawn = 0;
  if (millis() < nextSpawn) return;

  uint32_t avgMs = (uint32_t)EFFECT_INTERVAL_S * 1000UL;
  nextSpawn = millis() + random(avgMs / 2, avgMs * 3 / 2);

  activeEffect = pickEffect();

  // return;

  /** Effect cycling override */
  static size_t dbgIdx = 0;
  nextSpawn = millis() + 3000UL;  // Fast 3-second cycle window
  activeEffect = effects[dbgIdx++ % (sizeof(effects) / sizeof(effects[0]))].create();
}
