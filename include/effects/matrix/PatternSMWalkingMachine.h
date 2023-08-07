#pragma once

#include "effectmanager.h"
#include "effects/strip/musiceffect.h"

// Derived from https://editor.soulmatelights.com/gallery/1990-walking-machine

#if ENABLE_AUDIO
class PatternSMWalkingMachine : public BeatEffectBase,
                                public LEDStripEffect
#else
class PatternSMWalkingMachine : public LEDStripEffect
#endif
{
 private:
  // Walking machine
  // St3p40 aka Stepko
  // 11.04.23
  // Not for small matrices
  // Second name is "dreams in night"
  static constexpr int LED_COLS = MATRIX_WIDTH;
  static constexpr int LED_ROWS = MATRIX_HEIGHT;
#undef WU_WEIGHT
  static inline uint8_t WU_WEIGHT(uint8_t a, uint8_t b) {
    return (uint8_t)(((a) * (b) + (a) + (b)) >> 8);
  }

  void drawPixelXYF(float x, float y, const CRGB& color) {
    // extract the fractional parts and derive their inverses
    uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx,
            iy = 255 - yy;
    // calculate the intensities for each affected pixel
    uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy), WU_WEIGHT(ix, yy),
                     WU_WEIGHT(xx, yy)};
    // multiply the intensities by the colour, and saturating-add them to the
    // pixels
    for (uint8_t i = 0; i < 4; i++) {
      int16_t xn = x + (i & 1), yn = y + ((i >> 1) & 1);
      CRGB clr = g()->leds[g()->xy(xn, yn)];
      clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
      clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
      clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
      g()->leds[g()->xy(xn, yn)] = clr;
    }
#undef WU_WEIGHT
  }

  void drawCircleF(float cx, float cy, float radius, CRGB col) {
    uint8_t rad = radius;
    for (int8_t y = -radius; y < radius; y += 1) {
      for (int8_t x = -radius; x < radius; x += 1) {
        if (x * x + y * y < radius * radius) drawPixelXYF(cx + x, cy + y, col);
      }
    }
  }

  static float fmap(const float x, const float in_min, const float in_max,
                    const float out_min, const float out_max) {
    return (out_max - out_min) * (x - in_min) / (in_max - in_min) + out_min;
  }

  void drawLineF(float x1, float y1, float x2, float y2, const CRGB& col1,
                 const CRGB& col2) {
    float deltaX = fabs(x2 - x1);
    float deltaY = fabs(y2 - y1);
    float steps = 255 / max(deltaX, deltaY);
    float error = deltaX - deltaY;
    CRGB col = col1;
    float signX = x1 < x2 ? 0.5 : -0.5;
    float signY = y1 < y2 ? 0.5 : -0.5;

    while (x1 != x2 || y1 != y2) {
      if ((signX > 0. && x1 > x2 + signX) || (signX < 0. && x1 < x2 + signX))
        break;
      if ((signY > 0. && y1 > y2 + signY) || (signY < 0. && y1 < y2 + signY))
        break;
      drawPixelXYF(x1 /*+random(-10,10)*0.1 */, y1 /*+random(-10,10)*0.1*/,
                   nblend(col, col2, steps++));
      float error2 = error;
      if (error2 > -deltaY) {
        error -= deltaY;
        x1 += signX;
      }
      if (error2 < deltaX) {
        error += deltaX;
        y1 += signY;
      }
    }
  }
  struct {
    float posX;
    float posY;
  } dot[7];

 public:
  PatternSMWalkingMachine()
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(EFFECT_MATRIX_SMWALKING_MACHINE, "Walking Machine") {
  }

  PatternSMWalkingMachine(const JsonObjectConst& jsonObject)
      :
#if ENABLE_AUDIO
        BeatEffectBase(1.50, 0.05),
#endif
        LEDStripEffect(jsonObject) {
  }

  void Start() override { g()->Clear(); }

  void Draw() override {
#if ENABLE_AUDIO
    ProcessAudio();
#endif

    g()->Clear();

    for (byte i = 0; i < 7; i++) {
      dot[i].posX = (beatsin16(4, (LED_COLS >> 3) << 8,
                               (LED_COLS - (LED_COLS >> 3) - 1) << 8, i * 8192,
                               i * 8192)) /
                    255.f;
      dot[i].posY = (beatsin16(4, (LED_ROWS >> 3) << 8,
                               (LED_ROWS - (LED_ROWS >> 3) - 1) << 8, i * 4096,
                               16384 + i * 8192)) /
                    255.f;
    }
    for (uint8_t i = 0; i < 7; i++) {
      drawCircleF(dot[i].posX, dot[i].posY, 4, CHSV(i * 32, 255, 255));
      drawLineF(dot[i].posX, dot[i].posY, dot[(i + 1) % 7].posX,
                dot[(i + 1) % 7].posY, CHSV(i * 32, 255, 255),
                CHSV(((i + 1) % 7) * 32, 255, 255));
    }
  }

#if ENABLE_AUDIO
  void HandleBeat(bool bMajor, float elapsed, float span) override {}
#endif
};