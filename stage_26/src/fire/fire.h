#include <FastLED.h>

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

void Fire1D(CRGB *leds, uint8_t strip);

void update_fire();
