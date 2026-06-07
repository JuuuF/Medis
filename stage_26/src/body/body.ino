#include <FastLED.h>

// -----------------------------------------------------------------------
// Strip settings

#define LED_PIN 6

#define DEBUG

#ifdef DEBUG
#define CHIPSET WS2812
#define COLOR_ORDER GRB
#else
#define COLOR_ORDER BRG
#define CHIPSET WS2811
#endif  // DEBUG

#define MATRIX_HEIGHT 24
#define MATRIX_WIDTH 5
#define NUM_LEDS (MATRIX_HEIGHT * MATRIX_WIDTH)

#define BRIGHTNESS 255

// -----------------------------------------------------------------------
// Fire tuning
#define FIRESPEED 60          // How fast noise advances up the flame; higher = faster flickering flames
#define FIRENOISESCALE 110    // How flickery the fire is: 0 = twitchy; 255 = slow
#define FLAMEHEIGHT_RECIP 20  // How much to dim the flame: 0 = less dim/more flame; 255 = more dim/less flame

// -----------------------------------------------------------------------
// Smoke tuning
#define SMOKESPEED 1.1 * FIRESPEED  // How fast smoke advances up the flame; ideally a bit higher than FIRESPEED
#define SMOKENOISE_DIMMER 140       // Smoke dimming: 1 = thick smoke/more obscuring/flickery; 255 = compress smoke/less obscuring
#define SMOKENOISESCALE 200         // How flickery the smoke is: 0 = drift wide; 255 = smoke cuts out flame early

// -----------------------------------------------------------------------

#define CTRL1_STEP (11UL * 8)
#define CTRL2_STEP (13UL * 8)
#define FIRE_STEP (5UL * FIRESPEED * 8)
#define SMOKE_STEP (5UL * SMOKESPEED * 8)

uint32_t acc_ctrl1 = 0;
uint32_t acc_ctrl2 = 100000UL;  // matches original offset so noise starts varied
uint32_t acc_fire = 0;
uint32_t acc_smoke = 0;


CRGB leds[NUM_LEDS];
uint8_t heat[NUM_LEDS];

// -----------------------------------------------------------------------

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

void Fire1D(CRGB *leds, uint8_t strip);

// -----------------------------------------------------------------------

void setup() {
  pinMode(LED_PIN, OUTPUT);
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setDither(DISABLE_DITHER);

  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();
}

void loop() {

  EVERY_N_MILLISECONDS(8) {
    acc_ctrl1 += CTRL1_STEP;
    acc_ctrl2 += CTRL2_STEP;
    acc_fire += FIRE_STEP;
    acc_smoke += SMOKE_STEP;

    Fire1D(leds, 0);
    FastLED.show();
  }
}

// -----------------------------------------------------------------------

void Fire1D(CRGB *leds, uint8_t strip) {

  uint16_t ctrl1 = inoise16(acc_ctrl1, 0UL, 0UL);
  uint16_t ctrl2 = inoise16(acc_ctrl2, 0UL, 0UL);
  uint16_t ctrl = (ctrl1 >> 1) + (ctrl2 >> 1);

  uint32_t fire_nx = 3UL * ctrl * FIRESPEED;
  uint32_t fire_nz = acc_fire;
  uint8_t fire_scale = scale8(ctrl1, FIRENOISESCALE);

  uint32_t smoke_nx = 3UL * ctrl * (uint32_t)SMOKESPEED;
  uint32_t smoke_nz = acc_smoke;
  uint8_t smoke_scale = scale8(ctrl1, SMOKENOISESCALE);

  // Pass 1: build heat[] — fire noise computed per pixel, not stored
  heat[0] = max((uint8_t)(inoise16(fire_nx, fire_nz) >> 8), (uint8_t)120);
  for (uint8_t i = NUM_LEDS - 1; i > 0; i--) {
    heat[i] = (heat[i] + heat[i - 1] * 3) / 4;
  }
  heat[0] = inoise16(fire_nx, fire_nz) >> 8;  // restore raw base (two cheap calls, same result)

  // Pass 2: apply dim + palette + smoke — all noise computed inline
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    uint8_t fn = inoise16(fire_nx + (uint32_t)fire_scale * i, fire_nz) >> 8;
    uint16_t raw = inoise16(smoke_nx + (uint32_t)smoke_scale * i, smoke_nz);
    uint8_t smoke = (uint8_t)min(raw / SMOKENOISE_DIMMER, 255);

    heat[i] = scale8(heat[i], 255 - scale8(fn, FLAMEHEIGHT_RECIP));
    leds[i] = ColorFromPalette(hotPalette, heat[i], heat[i], LINEARBLEND);
    leds[i].nscale8(smoke);
  }
}
