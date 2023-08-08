#pragma once

#include "effectmanager.h"

// Derived from
// https://editor.soulmatelights.com/gallery/1895-agressivebouncingballs

class PatternSMGravityBalls : public LEDStripEffect
{
  private:
    static const uint8_t COUNT = 7;
    float posx[COUNT];
    float posy[COUNT];
    CRGB color;
    float velx[COUNT], vely[COUNT];
    float accel[COUNT];
    byte init = 1;

    static inline uint8_t WU_WEIGHT(uint8_t a, uint8_t b)
    {
        return (uint8_t)(((a) * (b) + (a) + (b)) >> 8);
    }

    void drawPixelXYF(float x, float y, CRGB color)
    {
        y = MATRIX_HEIGHT - y; // Compensate for mesmerizer's different coordiante system.
        if (!g()->isValidPixel(x, y))
            return;
        uint8_t xx = (x - (int)x) * 255, yy = (y - (int)y) * 255, ix = 255 - xx, iy = 255 - yy;
        // calculate the intensities for each affected pixel
        // #define WU_WEIGHT(a, b)((uint8_t)(((a) * (b) + (a) + (b)) >> 8))
        uint8_t wu[4] = {WU_WEIGHT(ix, iy), WU_WEIGHT(xx, iy), WU_WEIGHT(ix, yy), WU_WEIGHT(xx, yy)};
        // multiply the intensities by the colour, and saturating-add them to the
        // pixels
        for (uint8_t i = 0; i < 4; i++)
        {
            int16_t xn = x + (i & 1), yn = y + ((i >> 1) & 1);
            if (!g()->isValidPixel(xn, yn))
	    {
		// Log error, but for now, mostly just carry on
                return;
	    }
            CRGB clr = g()->leds[XY(xn, yn)];
            clr.r = qadd8(clr.r, (color.r * wu[i]) >> 8);
            clr.g = qadd8(clr.g, (color.g * wu[i]) >> 8);
            clr.b = qadd8(clr.b, (color.b * wu[i]) >> 8);
            if (!g()->isValidPixel(xn, yn))
	    {
		// Log different error, but for now, mostly just carry on
                return;
	    }
            g()->leds[XY(xn, yn)] = clr;
        }
    }

    float newpos()
    {
        return random(0.1F, 22.0F);
    }

    float newvel()
    {
        return random(0.1F, 1.0F);
    }

  public:
    PatternSMGravityBalls() : LEDStripEffect(EFFECT_MATRIX_SMGRAVITY_BALLS, "Gravity Balls")
    {
    }

    PatternSMGravityBalls(const JsonObjectConst &jsonObject) : LEDStripEffect(jsonObject)
    {
    }

    void Start() override
    {
        g()->Clear();
        for (uint i = 0; i < COUNT; i++)
        {
            velx[i] = (beatsin16(random(1, 2) + vely[i], 0, MATRIX_WIDTH, MATRIX_HEIGHT) / MATRIX_HEIGHT) + 0.1F;
            vely[i] = (beatsin16(5 + vely[i], 0, MATRIX_WIDTH, MATRIX_HEIGHT) / MATRIX_HEIGHT) + 0.5F;
            posx[i] = random(1.01F, float(MATRIX_HEIGHT));
            posy[i] = random(1.0F, float(MATRIX_WIDTH));
            accel[i] = random(0.1F, 1.0F);
        }
    }

    void Draw() override
    {
        for (uint i = 0; i < COUNT; i++)
        {
            if (posx[i] < 1 || posx[i] > MATRIX_WIDTH - 2)
            {
                velx[i] = -velx[i];
                if (velx[i] * velx[i] > 5)
                {
                    velx[i] = newvel();
                }
                if (posx[i] < 0 || posx[i] > MATRIX_WIDTH)
                {
                    posx[i] = newpos();
                }
                // initvalues();
            }
            if (posy[i] < 1 || posy[i] > MATRIX_HEIGHT - 2)
            {
                vely[i] = -vely[i];
                if (vely[i] * vely[i] > 10)
                {
                    vely[i] = newvel();
                }
                if (posy[i] < 0 || posy[i] > MATRIX_HEIGHT)
                {
                    posy[i] = newpos();
                }
                // initvalues();
            }
            vely[i] += 0.1F;
            posx[i] = posx[i] - velx[i] + accel[i]; // accel[i] ;//  posx[i] = posx[i] + velx[i] *
                                                    // accel[i]; posy[i] = posy[i] + vely[i] * accel[i];
            posy[i] = posy[i] - vely[i] - accel[i];
            drawPixelXYF(posx[i], posy[i], ColorFromPalette(CloudColors_p, i * 80, 255, LINEARBLEND));
        }

        blur2d(g()->leds, MATRIX_WIDTH, MATRIX_HEIGHT, 15);
        //  fadeToBlackBy(g()->leds,NUM_LEDS,20);
        fadeAllChannelsToBlackBy(20);
    }
};
