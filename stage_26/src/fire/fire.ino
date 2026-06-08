#include <FastLED.h>

// -----------------------------------------------------------------------
// Strip settings

#define LED_PIN_1 3
#define LED_PIN_2 6
#define LED_PIN_3 9
#define LED_PIN_4 12

#define DEBUG

#ifdef DEBUG
#define COLOR_ORDER GRB
#define CHIPSET WS2812
#else
#define COLOR_ORDER BRG
#define CHIPSET WS2811
#endif
#define NUM_LEDS 40
#define BRIGHTNESS 255

// -----------------------------------------------------------------------
// Commmunication settings

#define RX_PIN 2

// Timing (microseconds)
#define BIT_PERIOD_US 1000

volatile bool newRxData = false;
volatile uint32_t rxData = 0;

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

#define NUM_LAYERS 2
#define FIRENOISE 0
#define SMOKENOISE 1
#define STRIP_NOISE_OFFSET 50000UL

// -----------------------------------------------------------------------
// Incremental noise accumulators — advanced by fixed deltas each frame,
// no dependency on millis() or any clock. uint32_t wrapping is harmless;
// inoise16 treats the space as toroidal so the effect continues seamlessly.

// Tuned to match the original feel at ~8ms per frame.
// Tweak these to change speed without touching the noise scale defines.
#define CTRL1_STEP (11UL * 8)
#define CTRL2_STEP (13UL * 8)
#define FIRE_STEP (5UL * FIRESPEED * 8)
#define SMOKE_STEP (5UL * SMOKESPEED * 8)

uint32_t acc_ctrl1 = 0;
uint32_t acc_ctrl2 = 100000UL;  // matches original offset so noise starts varied
uint32_t acc_fire = 0;
uint32_t acc_smoke = 0;

// -----------------------------------------------------------------------

CRGB leds_1[NUM_LEDS];
CRGB leds_2[NUM_LEDS];
CRGB leds_3[NUM_LEDS];
CRGB leds_4[NUM_LEDS];

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

void Fire1D(CRGB *leds, uint8_t strip);

// -----------------------------------------------------------------------

uint8_t receiveEffect() {
  // 1. Wait for start pulse to go low -> true synchronization point
  while (digitalRead(RX_PIN) == HIGH) {}

  // 2. Advance exactly to the dead-center of the first data bit (Bit 7)
  // We skip the remainder of the Start Bit (1000us) and move halfway into Bit 7 (+500us)
  // Total delay from falling edge to center of Bit 7 = 1500us
  delayMicroseconds(BIT_PERIOD_US + (BIT_PERIOD_US / 2));

  uint8_t receivedData = 0;

  // 3. Sample 8 data bits sequentially (MSB to LSB)
  for (uint8_t bit = 0; bit < 8; bit++) {
    // Read the current state of the RX pin
    bool readOne = (digitalRead(RX_PIN) == HIGH);

    // Shift first to open the slot, then append our bit
    receivedData <<= 1;
    if (readOne) {
      receivedData |= 0x01;
    }

    // Only delay if there are more bits left to sample.
    // Stopping on the last bit prevents us from hanging around during the stop bit
    // and causing a false double-read.
    if (bit < 7) {
      delayMicroseconds(BIT_PERIOD_US);
    }
  }

  return receivedData;
}


// -----------------------------------------------------------------------


void setup() {
  pinMode(LED_PIN_1, OUTPUT);
  pinMode(LED_PIN_2, OUTPUT);
  pinMode(LED_PIN_3, OUTPUT);
  pinMode(LED_PIN_4, OUTPUT);
  FastLED.addLeds<CHIPSET, LED_PIN_1, COLOR_ORDER>(leds_1, NUM_LEDS);
  FastLED.addLeds<CHIPSET, LED_PIN_2, COLOR_ORDER>(leds_2, NUM_LEDS);
  FastLED.addLeds<CHIPSET, LED_PIN_3, COLOR_ORDER>(leds_3, NUM_LEDS);
  FastLED.addLeds<CHIPSET, LED_PIN_4, COLOR_ORDER>(leds_4, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.setDither(DISABLE_DITHER);
  fill_solid(leds_1, NUM_LEDS, CRGB::Black);
  fill_solid(leds_2, NUM_LEDS, CRGB::Black);
  fill_solid(leds_3, NUM_LEDS, CRGB::Black);
  fill_solid(leds_4, NUM_LEDS, CRGB::Black);
  FastLED.show();

  // Communication setup
  pinMode(RX_PIN, INPUT);

  Serial.begin(9600);
}

void loop() {

  EVERY_N_MILLISECONDS(8) {
    acc_ctrl1 += CTRL1_STEP;
    acc_ctrl2 += CTRL2_STEP;
    acc_fire += FIRE_STEP;
    acc_smoke += SMOKE_STEP;

    Fire1D(leds_1, 0);
    Fire1D(leds_2, 1);
    Fire1D(leds_3, 2);
    Fire1D(leds_4, 3);
    FastLED.show();
  }

  if (digitalRead(RX_PIN) == HIGH) {
    uint8_t effectID = receiveEffect();
    Serial.print("Received effect: ");
    Serial.println(effectID);
  }
}

// -----------------------------------------------------------------------

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


void fill() {
  fill_solid(leds_1, NUM_LEDS, CRGB::White);
  fill_solid(leds_2, NUM_LEDS, CRGB::White);
  fill_solid(leds_3, NUM_LEDS, CRGB::White);
  fill_solid(leds_4, NUM_LEDS, CRGB::White);
  FastLED.show();
  delay(1000);
}

void playEffect(uint8_t id) {
  fill();
  // switch (id) {
  //   case 0: fill(); break;
  //   case 1: /* effect 1 */ break;
  //   // ... add your effects
  //   default: break;
  // }
}
