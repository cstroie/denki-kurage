#ifndef JELLYFISH_H
#define JELLYFISH_H

#include "config.h"
#include "types.h"
#include <Arduino.h>
#include <LovyanGFX.hpp>

void drawJellyfish(lgfx::LGFX_Sprite *canvas, Point2D bell[NUM_BELL_VERTICES],
                   Point2D tentacles[NUM_TENTACLES][TENTACLE_SEGMENTS],
                   ColorMode mode, bool wireframe);
uint16_t getJellyfishColor(ColorMode mode, float brightness = 1.0f);

#endif