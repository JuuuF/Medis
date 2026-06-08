#include "effects.h"

#include "effect_sparkle.h"
#include "effect_outline.h"

Effect* getEffect(uint8_t effectID) {
  switch (effectID) {
    case 1: return new SparkleEffect();
    case 2: return new OutlineEffect();
  }

  return nullptr;
}
