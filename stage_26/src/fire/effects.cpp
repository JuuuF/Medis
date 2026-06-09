#include "effects.h"

#include "effect_sparkle.h"
#include "effect_outline.h"
#include "effect_ember.h"
#include "effect_rebirth.h"

Effect* getEffect(uint8_t effectID) {
  switch (effectID) {
    case 1: return new SparkleEffect();
    case 2: return new OutlineEffect();
    case 3: return new EmberEffectSlave();
    case 4: return new RebirthSlaveEffect();
  }

  return nullptr;
}
