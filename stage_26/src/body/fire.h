#pragma once

#include <stdint.h>

// ----------------------------------------------

void update_fire_values();
void FireBody(uint8_t col);
void FireHead(uint8_t col);

// ----------------------------------------------
// Fire tuning

#define FIRESPEED 60          // How fast noise advances up the flame; higher = faster flickering flames
#define FIRENOISESCALE 110    // How flickery the fire is: 0 = twitchy; 255 = slow
#define FLAMEHEIGHT_RECIP 10  // How much to dim the flame: 0 = less dim/more flame; 255 = more dim/less flame

#define HEAD_FIRESPEED 120  // head flame a bit more energetic
#define HEAD_FIRENOISESCALE 90
#define HEAD_FLAMEHEIGHT_RECIP 10

// ----------------------------------------------
// Smoke tuning
#define SMOKESPEED 1.1 * FIRESPEED  // How fast smoke advances up the flame; ideally a bit higher than FIRESPEED
#define SMOKENOISE_DIMMER 140       // Smoke dimming: 1 = thick smoke/more obscuring/flickery; 255 = compress smoke/less obscuring
#define SMOKENOISESCALE 200         // How flickery the smoke is: 0 = drift wide; 255 = smoke cuts out flame early

#define HEAD_CTRL1_STEP (11UL * 8)
#define HEAD_CTRL2_STEP (13UL * 8)
#define HEAD_FIRE_STEP (5UL * HEAD_FIRESPEED * 8)

// ----------------------------------------------
// Accumulator steps
#define CTRL1_STEP (11UL * 8)
#define CTRL2_STEP (13UL * 8)
#define FIRE_STEP (5UL * FIRESPEED * 8)
#define SMOKE_STEP (5UL * SMOKESPEED * 8)

// ----------------------------------------------
// Column-wise noise offset
#define COL_NOISE_OFFSET 50000UL
#define HEAD_NOISE_OFFSET 99999UL

#define FLAME_HEIGHT (MATRIX_HEIGHT - HEAD_LEDS)
