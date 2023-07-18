#pragma once

#include "effects/strip/musiceffect.h"
#include "effectmanager.h"

// Derived from https://editor.soulmatelights.com/gallery/1245-rainbow-swirl
// Super simple code, but quite pleasant.

#if ENABLE_AUDIO
class PatternSMRainbowSwirl : public BeatEffectBase, public LEDStripEffect
#else
class PatternSMRainbowSwirl : public LEDStripEffect
#endif
{
private:
public:
  PatternSMRainbowSwirl() :
#if ENABLE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(EFFECT_MATRIX_SMRAINBOW_SWIRL, "Rainbow Swirl")
    {
    }

  PatternSMRainbowSwirl(const JsonObjectConst& jsonObject) :
#if ENABLE_AUDIO
    BeatEffectBase(1.50, 0.05),
#endif
    LEDStripEffect(jsonObject)
  {
  }

  virtual void Start() override
  {
    g()->Clear();
  }

  virtual void Draw() override
  {
#if ENABLE_AUDIO
    ProcessAudio();
#endif
    for (int x = 0; x < MATRIX_WIDTH; x++) {
      for (int y = 0; y < MATRIX_HEIGHT; y++) {
        // XY tells us the index of a given X/Y coordinate
        int index = g()->xy(x, y);
        int hue = x * 10 + y * 10;
        hue += sin8(millis() / 50 + y * 5 + x * 7);
        g()->leds[index] = CHSV(hue, 255, 255);
    }
  }
  }

#if ENABLE_AUDIO
  virtual void HandleBeat(bool bMajor, float elapsed, float span) override
  {

  }
#endif
};