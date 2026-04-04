#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

#define CL_BG 0x0000

const int BTN_PIN = 0;

#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

#define TFT_BL_PIN 21

const uint16_t TOUCH_MIN_X = 200;
const uint16_t TOUCH_MAX_X = 3800;
const uint16_t TOUCH_MIN_Y = 250;
const uint16_t TOUCH_MAX_Y = 3850;

const int BELL_RINGS = 4;
const int BELL_POINTS_PER_RING = 12;
const int NUM_TENTACLES = 12;
const int TENTACLE_SEGMENTS = 8;
const int NUM_BELL_VERTICES = BELL_RINGS * BELL_POINTS_PER_RING + 1;

#define NUM_PARTICLES 50

#define LED_R_PIN 4
#define LED_G_PIN 16
#define LED_B_PIN 17

#endif