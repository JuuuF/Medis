#include "effects.h"

#include "effect_sparkle.h"
#include "effect_outline.h"
#include "effect_ember.h"
#include "effect_rebirth.h"
#include "effect_feather.h"
#include "effect_pride.h"

Effect* getEffect(uint8_t effectID) {
  switch (effectID) {
    case 1: return new SparkleEffect();
    case 2: return new OutlineEffect();
    case 3: return new EmberEffectSlave();
    case 4: return new RebirthSlaveEffect();
    case 5: return new FeatherCascadeSlaveEffect();
    case 6: return new PrideCascadeSlaveEffect();
  }

  return nullptr;
}
