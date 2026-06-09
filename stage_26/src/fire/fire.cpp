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

uint32_t acc_ctrl1 = 0;
uint32_t acc_ctrl2 = 100000UL;  // matches original offset so noise starts varied
uint32_t acc_fire = 0;
uint32_t acc_smoke = 0;

static uint8_t fireNoise[NUM_LEDS];
static uint8_t smokeNoise[NUM_LEDS];
static uint8_t heatBuf[NUM_LEDS];

void update_fire() {
  acc_ctrl1 += CTRL1_STEP;
  acc_ctrl2 += CTRL2_STEP;
  acc_fire += FIRE_STEP;
  acc_smoke += SMOKE_STEP;
}

void Fire1D(CRGB *leds, uint8_t strip) {
  uint32_t offset = (uint32_t)strip * STRIP_NOISE_OFFSET;

  // ── Control signals ──────────────────────────────────────────────────────
  uint16_t ctrl1 = inoise16(acc_ctrl1 + offset, 0UL, 0UL);
  uint16_t ctrl2 = inoise16(acc_ctrl2 + offset, 0UL, 0UL);
  uint16_t ctrl = (ctrl1 >> 1) + (ctrl2 >> 1);

  // ── Fire noise layer → fireNoise[] ───────────────────────────────────────
  uint32_t fire_nx = 3UL * ctrl * FIRESPEED;
  uint32_t fire_nz = acc_fire + offset;
  uint8_t fire_scale = scale8(ctrl1, FIRENOISESCALE);

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint32_t off = (uint32_t)fire_scale * i;
    fireNoise[i] = inoise16(fire_nx + off, fire_nz) >> 8;
  }

  // ── Smoke noise layer → smokeNoise[] ─────────────────────────────────────
  uint32_t smoke_nx = 3UL * ctrl * SMOKESPEED;
  uint32_t smoke_nz = acc_smoke + offset;
  uint8_t smoke_scale = scale8(ctrl1, SMOKENOISESCALE);

  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint32_t off = (uint32_t)smoke_scale * i;
    uint16_t raw = inoise16(smoke_nx + off, smoke_nz);
    smokeNoise[i] = (uint8_t)min((uint16_t)(raw / SMOKENOISE_DIMMER), (uint16_t)255);
  }

  // ── Heat diffusion into heatBuf[] ────────────────────────────────────────
  heatBuf[0] = max(fireNoise[0], (uint8_t)120);
  for (uint8_t i = NUM_LEDS - 1; i > 0; i--) {
    heatBuf[i] = (heatBuf[i] + heatBuf[i - 1] * 3) >> 2;  // (1×cur + 3×prev) / 4
  }
  heatBuf[0] = fireNoise[0];

  // ── Height dimming + palette mapping ─────────────────────────────────────
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t dim = 255 - scale8(fireNoise[i], FLAMEHEIGHT_RECIP);
    heatBuf[i] = scale8(heatBuf[i], dim);

    leds[i] = ColorFromPalette(hotPalette, heatBuf[i], heatBuf[i], LINEARBLEND);
    leds[i].nscale8(smokeNoise[i]);
  }
}
