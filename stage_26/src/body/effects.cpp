#include "fl/stl/stdint.h"
#include "effects.h"

#include "config.h"
#include "mapping.h"

#include "effect_sparkle.h"
#include "effect_outline.h"
#include "effect_ember.h"
#include "effect_rebirth.h"
#include "effect_feather.h"
#include "effect_pride.h"
#include "effect_ascension.h"
#include "effect_fawkesweeps.h"
#include "effect_breath.h"

#include <FastLED.h>

// ----------------------------------------------

struct EffectEntry {
  uint8_t effectID;
  uint8_t weight;
};

// Selection Table Pool
const EffectEntry effects[] = {
  { 0, 5 },  // SparkleEffect
  { 1, 2 },  // OutlineEffect
  { 2, 4 },  // EmberEffect
  { 3, 2 },  // RebirthEffect
  { 4, 2 },  // FeatherCascadeEffect
  { 5, 1 },  // PrideCascadeEffect
  { 7, 1 },  // CelestialAscensionEffect
  { 8, 3 },  // FawkesWeepsEffect
  { 9, 2 }   // SacredBreathEffect
};

#define EFFECT_COUNT (sizeof(effects) / sizeof(effects[0]))

Effect* createEffectByID(uint8_t id) {
  switch (id) {
    case 0: return new SparkleEffect(id);
    case 1: return new OutlineEffect(id);
    case 2: return new EmberEffect(id);
    case 3: return new RebirthEffect(id);
    case 4: return new FeatherCascadeEffect(id);
    case 5: return new PrideCascadeEffect(id);
    case 7: return new CelestialAscensionEffect(id);
    case 8: return new FawkesWeepsEffect(id);
    case 9: return new SacredBreathEffect(id);

    default:
      // Safe fallback mechanism if an invalid ID enters the framework
      return new SparkleEffect(0);
  }
}

Effect* pickEffect() {
  uint16_t total = 0;
  for (uint8_t i = 0; i < EFFECT_COUNT; i++) {
    total += effects[i].weight;
  }

  uint16_t pick = random(total);
  uint16_t cumulative = 0;

  for (uint8_t i = 0; i < EFFECT_COUNT; i++) {
    cumulative += effects[i].weight;
    if (pick < cumulative) {
      Serial.print("Playing effect ID: ");
      Serial.println(effects[i].effectID);

      // Query our map with the chosen ID
      return createEffectByID(effects[i].effectID);
    }
  }

  return createEffectByID(effects[0].effectID);
}

void maybeSpawnEffect() {
  if (EFFECT_INTERVAL_S == 0) return;
  if (activeEffect != nullptr) return;

  static uint32_t nextSpawn = 0;
  if (millis() < nextSpawn) return;

  uint32_t avgMs = (uint32_t)EFFECT_INTERVAL_S * 1000UL;
  nextSpawn = millis() + random(avgMs / 2, avgMs * 3 / 2);
  activeEffect = pickEffect();

#ifdef DEBUG
  /** Sequential Cycle Tool */
  static size_t dbgIdx = 0;
  nextSpawn = millis() + 3000UL;

  uint8_t targetID = effects[dbgIdx % EFFECT_COUNT].effectID;
  dbgIdx++;

  delete activeEffect;
  activeEffect = createEffectByID(targetID);
#endif
}
