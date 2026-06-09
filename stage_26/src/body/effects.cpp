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
#include "effect_fireblessing.h"

#include <FastLED.h>

// ----------------------------------------------

struct EffectEntry {
  uint8_t effectID;
  uint8_t weight;
};

// Selection Table Pool
const EffectEntry effects[] = {
  { 1, 5 },   // SparkleEffect
  { 2, 2 },   // OutlineEffect
  { 3, 4 },   // EmberEffect
  { 4, 2 },   // RebirthEffect
  { 5, 2 },   // FeatherCascadeEffect
  { 6, 1 },   // PrideCascadeEffect
  { 7, 1 },   // CelestialAscensionEffect
  { 8, 3 },   // FawkesWeepsEffect
  { 9, 2 },   // SacredBreathEffect
  { 10, 2 },  // FireBlessing
};

#define EFFECT_COUNT (sizeof(effects) / sizeof(effects[0]))

Effect* createEffectByID(uint8_t id) {
  switch (id) {
    case 1: return new SparkleEffect(id);
    case 2: return new OutlineEffect(id);
    case 3: return new EmberEffect(id);
    case 4: return new RebirthEffect(id);
    case 5: return new FeatherCascadeEffect(id);
    case 6: return new PrideCascadeEffect(id);
    case 7: return new CelestialAscensionEffect(id);
    case 8: return new FawkesWeepsEffect(id);
    case 9: return new SacredBreathEffect(id);
    case 10: return new FireBlessingEffect(id);

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
  // Don't choose an effect if interval is 0
  if (EFFECT_INTERVAL_S == 0) return;

  // Wait for current effect to finish
  if (activeEffect != nullptr) return;

#ifdef DEBUG
  // --- DEBUG MODE: Sequential Cycle ---
  static size_t dbgIdx = 0;
  uint8_t targetID = effects[dbgIdx % EFFECT_COUNT].effectID;
  dbgIdx++;

  Serial.print("[DEBUG] Next sequential effect triggered. ID: ");
  Serial.println(targetID);

  activeEffect = createEffectByID(targetID);

#else
  // --- PRODUCTION MODE: Weighted Random Spawn with Timing Delay ---
  static uint32_t nextSpawn = 0;
  if (millis() < nextSpawn) return;

  uint32_t avgMs = (uint32_t)EFFECT_INTERVAL_S * 1000UL;
  nextSpawn = millis() + random(avgMs / 2, avgMs * 3 / 2);

  activeEffect = pickEffect();
#endif
}