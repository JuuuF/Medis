#include "fire.h"

#include "config.h"

DEFINE_GRADIENT_PALETTE(hot_gp){
  0, 0, 0, 0,          // 0x000000
  17, 51, 0, 0,        // 0x330000
  34, 102, 0, 0,       // 0x660000
  51, 153, 0, 0,       // 0x990000
  68, 204, 0, 0,       // 0xCC0000
  85, 255, 0, 0,       // 0xFF0000
  102, 255, 51, 0,     // 0xFF3300
  119, 255, 102, 0,    // 0xFF6600
  136, 255, 153, 0,    // 0xFF9900
  153, 255, 204, 0,    // 0xFFCC00
  170, 255, 255, 0,    // 0xFFFF00
  187, 255, 255, 51,   // 0xFFFF33
  204, 255, 255, 102,  // 0xFFFF66
  221, 255, 255, 153,  // 0xFFFF99
  238, 255, 255, 204,  // 0xFFFFCC
  255, 255, 255, 255   // 0xFFFFFF
};
DEFINE_GRADIENT_PALETTE(custom_fire_gp){
  0, 0, 0, 0,        // #000000
  20, 40, 8, 0,      // dark ember
  40, 120, 25, 0,    // dark orange-red
  80, 210, 74, 0,    // #d24a00
  120, 255, 90, 0,   // #ff5a00
  160, 255, 154, 0,  // #ff9a00
  180, 255, 206, 0,  // #ffce00
  220, 255, 232, 0,  // #ffe800
  240, 255, 255, 0,  // #ffff00
  255, 255, 255, 255
};

CRGBPalette32 hotPalette = custom_fire_gp;

uint32_t nx[NUM_LAYERS];
uint32_t nz[NUM_LAYERS];
uint32_t scale_n[NUM_LAYERS];

uint8_t noise[4][NUM_LAYERS][NUM_LEDS];
uint8_t heat[4][NUM_LEDS];

uint32_t acc_ctrl1 = 0;
uint32_t acc_ctrl2 = 100000UL;  // matches original offset so noise starts varied
uint32_t acc_fire = 0;
uint32_t acc_smoke = 0;

void update_fire() {
  acc_ctrl1 += CTRL1_STEP;
  acc_ctrl2 += CTRL2_STEP;
  acc_fire += FIRE_STEP;
  acc_smoke += SMOKE_STEP;
}

void Fire1D(CRGB *leds, uint8_t strip) {
  uint32_t offset = strip * STRIP_NOISE_OFFSET;

  uint16_t ctrl1 = inoise16(acc_ctrl1 + offset, 0UL, 0UL);
  uint16_t ctrl2 = inoise16(acc_ctrl2 + offset, 0UL, 0UL);
  uint16_t ctrl = (ctrl1 >> 1) + (ctrl2 >> 1);

  nx[FIRENOISE] = 3UL * ctrl * FIRESPEED;
  nz[FIRENOISE] = acc_fire + offset;
  scale_n[FIRENOISE] = scale8(ctrl1, FIRENOISESCALE);

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint32_t off = (uint32_t)scale_n[FIRENOISE] * i;
    noise[strip][FIRENOISE][i] =
      inoise16(nx[FIRENOISE] + off, nz[FIRENOISE]) >> 8;
  }

  nx[SMOKENOISE] = 3UL * ctrl * SMOKESPEED;
  nz[SMOKENOISE] = acc_smoke + offset;
  scale_n[SMOKENOISE] = scale8(ctrl1, SMOKENOISESCALE);

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint32_t off = (uint32_t)scale_n[SMOKENOISE] * i;
    uint16_t raw = inoise16(nx[SMOKENOISE] + off, nz[SMOKENOISE]);
    noise[strip][SMOKENOISE][i] = min(raw / SMOKENOISE_DIMMER, 255);
  }

  heat[strip][0] = max(noise[strip][FIRENOISE][0], 120);
  for (uint8_t i = NUM_LEDS - 1; i > 0; i--) {
    heat[strip][i] = (heat[strip][i] * 1 + heat[strip][i - 1] * 3) / 4;
    // (heat[strip][i - 1] * 3 + heat[strip][max(i-2, 0)]) / 4;
  }

  heat[strip][0] = noise[strip][FIRENOISE][0];

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t dim = 255 - scale8(noise[strip][FIRENOISE][i], FLAMEHEIGHT_RECIP);
    heat[strip][i] = scale8(heat[strip][i], dim);

    leds[i] = ColorFromPalette(hotPalette, heat[strip][i], heat[strip][i], LINEARBLEND);
    leds[i].nscale8(noise[strip][SMOKENOISE][i]);
  }
}
