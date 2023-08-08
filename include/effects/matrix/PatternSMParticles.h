#pragma once

#include "effectmanager.h"

// Derived from
// https://editor.soulmatelights.com/gallery/2358-particles
//
// I have stared and stared at this code. I'm not at all convinced
// this was designed as much as someone typed magic numbers that
// looked "good" on their screen geometry.
//
// 'now_weekMs' relies on millis() generating basically a raster dot
//  that indexes into leds[]
//
// It generates wildly out-of-bounds accesses and _relies_ on
// drawPixels() to throw them away. Over half the drawn pixels are
// discarded in some geometries. I found the original had X and Y
// swapped in a few places. Fixing that REALLY helped our rectangular
// screens. The draw loop does a RIDICULOUS amount of math just to
// add +/1 randomly to pixels. (Or just keep them as floats, add whatever
// to them, and just snap to the integer grid on draw, which sould allow
// dithering....)
//
// I could (and probably should) have rewritten this in less time.

// Good freakin' grief.
#define now_weekMs 0 * 1000ul + millis() // - tmr

class PatternSMParticles : public LEDStripEffect
{
  private:
    const int thisScale = 254; // 254 - максимальный, наверное, масштаб
    const int CUR_PRES_speed = 25; // 15 - скорость, на которой частицы пляшут туда-сюда
    const int CUR_PRES_color = 1; // 1 - цвет частиц, чтобы не заморачиваться с палитрами

    const int cfg_length = MATRIX_HEIGHT;
    const int cfg_width = MATRIX_WIDTH;
    const int Xnow_weekMs = 0 * 1000ul + millis(); // - tmr

  public:
    PatternSMParticles() : LEDStripEffect(EFFECT_MATRIX_SMPARTICLES, "Particles")
    {
    }

    PatternSMParticles(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
    }

    void drawPixelXY(uint8_t x, uint8_t y, CRGB color)
    {
        if (!g()->isValidPixel(x, MATRIX_HEIGHT - 1 - y))
            return;
        uint32_t thisPixel = XY(x, MATRIX_HEIGHT - 1 - y);
        g()->leds[thisPixel] = color;
    } // служебные функции

    void Draw() override
    {
        fadeAllChannelsToBlackBy(80);

        // This relies on 'now_weekMs' changing on every loop iteration.
        byte amount = (thisScale >> 3) + 1;
        for (int i = 0; i < amount; i++)
        {
            int homeX = inoise16(i * 100000000ul + (now_weekMs << 3) * CUR_PRES_speed / 255);
            homeX = map(homeX, 15000, 50000, 0, cfg_width);
            int offsX = inoise8(i * 2500 + (now_weekMs >> 1) * CUR_PRES_speed / 255) - 128;
            offsX = cfg_width / 2 * offsX / 128;
            unsigned int thisX = homeX + offsX;

            int homeY = inoise16(i * 100000000ul + 2000000000ul + (now_weekMs << 3) * CUR_PRES_speed / 255);
            homeY = map(homeY, 15000, 50000, 0, cfg_length);
            int offsY = inoise8(i * 2500 + 30000 + (now_weekMs >> 1) * CUR_PRES_speed / 255) - 128;
            offsY = cfg_length / 2 * offsY / 128;
            unsigned int thisY = homeY + offsY;
            drawPixelXY(thisX, thisY, CRGB(CHSV(CUR_PRES_color, 255, 255)));
        }
    }
};