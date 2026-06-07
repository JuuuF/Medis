#include "fire.h"

#include "config.h"
#include "mapping.h"

// ----------------------------------------------

uint32_t acc_ctrl1 = 0;
uint32_t acc_ctrl2 = 100000UL;  // matches original offset so noise starts varied
uint32_t acc_fire = 0;
uint32_t acc_smoke = 0;

uint32_t acc_head_ctrl1 = 50000UL;  // offset so head noise starts different from body
uint32_t acc_head_ctrl2 = 150000UL;
uint32_t acc_head_fire = 0;

// ----------------------------------------------

uint8_t heat[MATRIX_WIDTH][MATRIX_HEIGHT];
uint8_t heat_head[MATRIX_WIDTH][HEAD_LEDS];

// ----------------------------------------------

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

// ----------------------------------------------

void update_fire_values() {
  acc_ctrl1 += CTRL1_STEP;
  acc_ctrl2 += CTRL2_STEP;
  acc_fire += FIRE_STEP;

  acc_head_ctrl1 += HEAD_CTRL1_STEP;
  acc_head_ctrl2 += HEAD_CTRL2_STEP;
  acc_head_fire += HEAD_FIRE_STEP;

  acc_smoke += SMOKE_STEP;
}

void FireBody(uint8_t col) {

  uint32_t offset = (uint32_t)col * COL_NOISE_OFFSET;

  uint16_t ctrl1 = inoise16(acc_ctrl1 + offset, 0UL, 0UL);
  uint16_t ctrl2 = inoise16(acc_ctrl2 + offset, 0UL, 0UL);
  uint16_t ctrl = (ctrl1 >> 1) + (ctrl2 >> 1);

  uint32_t fire_nx = 3UL * ctrl * FIRESPEED;
  uint32_t fire_nz = acc_fire + offset;
  uint8_t fire_scale = scale8(ctrl1, FIRENOISESCALE);

  uint32_t smoke_nx = 3UL * ctrl * (uint32_t)SMOKESPEED;
  uint32_t smoke_nz = acc_smoke + offset;
  uint8_t smoke_scale = scale8(ctrl1, SMOKENOISESCALE);

  // Pass 1: seed base row, propagate downward (decreasing row index).
  uint8_t base = FLAME_HEIGHT - 1;
  heat[col][base] = max((uint8_t)(inoise16(fire_nx, fire_nz) >> 8), (uint8_t)120);
  for (uint8_t i = 0; i < base; i++) {  // i=0 is tip
    heat[col][i] = (heat[col][i] + heat[col][i + 1] * 3) / 4;
  }
  heat[col][base] = inoise16(fire_nx, fire_nz) >> 8;

  // Pass 2: dim + palette + smoke
  for (uint8_t i = 0; i < FLAME_HEIGHT; i++) {
    uint8_t fn = inoise16(fire_nx + (uint32_t)fire_scale * i, fire_nz) >> 8;
    uint16_t raw = inoise16(smoke_nx + (uint32_t)smoke_scale * i, smoke_nz);
    uint8_t smoke = (uint8_t)min(raw / SMOKENOISE_DIMMER, (uint16_t)255);

    heat[col][i] = scale8(heat[col][i], 255 - scale8(fn, FLAMEHEIGHT_RECIP));

    // y=0 is now the physical bottom; fire zone sits below the head.
    // Offset by HEAD_LEDS so fire rows map to the lower portion of the matrix.
    uint16_t idx = XY(col, i);
    leds[idx] = ColorFromPalette(hotPalette, heat[col][i], heat[col][i], LINEARBLEND);
    leds[idx].nscale8(smoke);
  }
}

void FireHead(uint8_t col) {
  uint32_t offset = (uint32_t)col * HEAD_NOISE_OFFSET;

  uint16_t ctrl1 = inoise16(acc_head_ctrl1 + offset, 0UL, 0UL);
  uint16_t ctrl2 = inoise16(acc_head_ctrl2 + offset, 0UL, 0UL);
  uint16_t ctrl = (ctrl1 >> 1) + (ctrl2 >> 1);

  uint32_t fire_nx = 3UL * ctrl * HEAD_FIRESPEED;
  uint32_t fire_nz = acc_head_fire + offset;
  uint8_t fire_scale = scale8(ctrl1, HEAD_FIRENOISESCALE);

  // Seed at row 0 (boundary), propagate upward toward HEAD_LEDS-1
  heat_head[col][0] = max((uint8_t)(inoise16(fire_nx, fire_nz) >> 8), (uint8_t)120);
  for (uint8_t i = HEAD_LEDS - 1; i > 0; i--) {
    heat_head[col][i] = (heat_head[col][i] + heat_head[col][i - 1] * 3) / 4;
  }
  heat_head[col][0] = inoise16(fire_nx, fire_nz) >> 8;

  // No smoke on the head — just fire palette with height dimming
  for (uint8_t i = 0; i < HEAD_LEDS; i++) {
    uint8_t fn = inoise16(fire_nx + (uint32_t)fire_scale * i, fire_nz) >> 8;
    heat_head[col][i] = scale8(heat_head[col][i], 255 - scale8(fn, HEAD_FLAMEHEIGHT_RECIP));

    uint16_t idx = XY(col, FLAME_HEIGHT + i);
    leds[idx] = ColorFromPalette(hotPalette, heat_head[col][i], heat_head[col][i], LINEARBLEND);
  }
}
