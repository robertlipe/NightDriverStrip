#ifndef PATTERN_CG_KONG_H
#define PATTERN_CG_KONG_H

#include "globals.h"
#include <vector>
#include <chrono>
#include <algorithm>
#include <cmath>

// #define debugEffect if (false) debugA
#define debugEffect if (true) debugA


class PatternCGKong : public EffectWithId<PatternCGKong>
{
    struct Barrel {
        float x, y;
        float vx, vy;
        bool active;
        bool isBlue;
    };

    // Physics & Game Constants
    static constexpr float kGravity = 0.15f;
    static constexpr float kJumpStrength = -1.1f; // v30: Restored to original high jump to clear barrels
    static constexpr float kWalkSpeed = 0.7f;
    static constexpr float kClimbSpeed = 0.4f;

    // Layout & Threshold Constants
    static constexpr float kGroundY = 31.0f;
    static constexpr float kT1Y = 27.0f;
    static constexpr float kT2Y = 23.0f;
    static constexpr float kT3Y = 17.0f;
    static constexpr float kDkPlatformY = 13.0f;

    // v18: Derived Thresholds (No Magic Numbers)
    // Used to determine if Mario "fell" below a tier.
    static constexpr float kThreshG = kT1Y - 1.0f; // > 26
    static constexpr float kThreshT1 = kT2Y - 3.0f; // > 20
    static constexpr float kThreshT2 = kT3Y - 2.0f; // > 15

    // Ladder positions
    static constexpr float kLadder1X = 8.0f;
    static constexpr float kLadder2X = 32.0f; // MATRIX_WIDTH / 2
    static constexpr float kLadder3X = 56.0f; // MATRIX_WIDTH - 8
    static constexpr float kLadder4X = 12.0f;

    // Safety & Tolerance Constants
    static constexpr float kLadderSnapDist = 2.0f;
    static constexpr float kLadderProximity = 4.0f;
    static constexpr float kBailProximity = 6.0f;
    static constexpr float kWaitOffset = 10.0f;
    static constexpr float kWaitOffsetLarge = 12.0f;
    static constexpr float kSafetyRadSmall = 6.0f;
    static constexpr float kSafetyRadLarge = 12.0f;
    static constexpr float kSafetyRadHuge = 20.0f;

    static constexpr int kLadderWidth = 4;

    enum EntityState { WALKING, CLIMBING, JUMPING };
    struct Entity {
        float x = 0.0f;
        float y = 0.0f;
        uint8_t frame = 0;
        uint32_t lastAnimTime = 0;
        EntityState state = WALKING;
        float vx = 0.0f;
        float vy = 0.0f;
        float targetX = 0.0f;
        bool faceLeft = false;
        float jumpFloorY = 28.0f;
        float climbTargetY = 0.0f;
        int tier = 0; // v30: Explicit tier tracking
        uint32_t lastClimbTime = 0; // v32: Cooldown to prevent ladder jitter
        uint32_t panicTime = 0; // v34: Retreat persistence
	bool _wasJumping {false};
    };

    std::vector<Barrel> _barrels;
    Entity _dk;
    Entity _mario;
    int _lastMin = -1;
    float _flashAmount = 0.0f;
    bool _gameActive = true;
    uint32_t _winTime = 0; // v16: Time of victory
    uint32_t _resetTime = 0; // v37: Fade reset timer
    uint32_t _tier0Time = 0; // v37.2: Track time on ground floor
    float _dkBreath;
// FIXME: rename to _frameTime
    uint32_t frameTime;

    // 7x12 font (scaffolding style)
    static constexpr uint16_t font7x12[11][12] = {
        {0x1C, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1C}, // 0
        {0x08, 0x18, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1C}, // 1
        {0x1C, 0x22, 0x22, 0x02, 0x02, 0x1C, 0x20, 0x20, 0x20, 0x20, 0x22, 0x3E}, // 2
        {0x1C, 0x22, 0x22, 0x02, 0x02, 0x1C, 0x02, 0x02, 0x02, 0x02, 0x22, 0x1C}, // 3
        {0x04, 0x0C, 0x1C, 0x24, 0x44, 0x44, 0x3F, 0x04, 0x04, 0x04, 0x04, 0x04}, // 4
        {0x3E, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x02, 0x02, 0x02, 0x02, 0x22, 0x1C}, // 5
        {0x1C, 0x22, 0x20, 0x20, 0x20, 0x3C, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1C}, // 6
        {0x3E, 0x22, 0x02, 0x02, 0x04, 0x08, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10}, // 7
        {0x1C, 0x22, 0x22, 0x22, 0x22, 0x1C, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1C}, // 8
        {0x1C, 0x22, 0x22, 0x22, 0x22, 0x1E, 0x02, 0x02, 0x02, 0x02, 0x22, 0x1C}, // 9
        {0x00, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x00, 0x00}  // :
    };

    // DK Fur (Dark Brown) 12x12 - Gorilla silhouette v9 (Arm gaps for cleaner look)
    static constexpr uint16_t dkFur[3][12] = {
        {0x000, 0x0E0, 0x1F0, 0x3F8, 0x7FC, 0x7FC, 0x7FC, 0xFFF, 0xFFF, 0x7FC, 0x3F8, 0x228}, // Standing (hunch)
        {0x000, 0x0E0, 0x1F0, 0x7FC, 0xFFF, 0xC3F, 0xC3F, 0xC3F, 0xFFF, 0x7FC, 0x666, 0x666}, // Pounding A (Arms out, wide stance)
        {0x7FC, 0xFFF, 0xC3F, 0xC3F, 0xC3F, 0xFFF, 0x7FC, 0x7FC, 0x7FC, 0x3F8, 0x228, 0x666}  // Pounding B (Arms up, wide stance - Feet fixed!)
    };
    // DK Skin (Tan) - Facial features v7
    static constexpr uint16_t dkTan[3][12] = {
        {0x000, 0x000, 0x0A0, 0x000, 0x1F0, 0x1F0, 0x000, 0x1F0, 0x1F0, 0x0C0, 0x000, 0x000},
        {0x000, 0x000, 0x0A0, 0x000, 0x1F0, 0x1F0, 0x000, 0x1F0, 0x1F0, 0x0C0, 0x000, 0x000},
        {0x000, 0x000, 0x0A0, 0x000, 0x1F0, 0x1F0, 0x000, 0x3F0, 0x3F0, 0x1E0, 0x000, 0x000}
    };

    // Mario Layers (8x8)
    static constexpr uint8_t marioRed[4][8] = {
        {0x38, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // Hat/Shirt Stand
        {0x38, 0x7C, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00}, // Run A
        {0x38, 0x7C, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00}, // Run B
        {0x38, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // Climb
    };
    static constexpr uint8_t marioBlue[4][8] = {
        {0x00, 0x00, 0x00, 0x00, 0x18, 0x3C, 0x24, 0x00}, // Overalls
        {0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x1C, 0x00}, // Run A
        {0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x38, 0x00}, // Run B
        {0x00, 0x00, 0x00, 0x00, 0x3C, 0x3C, 0x3C, 0x42}  // Climb
    };
    static constexpr uint8_t marioSkin[4][8] = {
        {0x00, 0x00, 0x3C, 0x7E, 0x00, 0x00, 0x00, 0x00}, // Face/Hands
        {0x00, 0x00, 0x3C, 0x7E, 0x42, 0x42, 0x00, 0x00}, // Run A
        {0x00, 0x00, 0x3C, 0x7E, 0x42, 0x42, 0x00, 0x00}, // Run B
        {0x00, 0x00, 0x3C, 0x7E, 0x00, 0x00, 0x00, 0x00}  // Climb
    };

    static constexpr uint32_t kRed = 0x440000; // Darker Crimson
					       //
    // CGKong palette expressed as RGB565 *intent*.
    // Conversion to CRGB is gamma-corrected at draw time.
    static constexpr uint16_t kDKFur565     = 0xA145;
    static constexpr uint16_t kDKTan565     = 0xF6B2;
    static constexpr uint16_t kMarioSkin565 = 0xFBA0;
    static constexpr uint16_t kBarrel565    = 0xC618;
    static constexpr uint16_t kGirder565    = 0xD800;

public:
    PatternCGKong() : EffectWithId<PatternCGKong>("CG Kong") { ResetGame(); }
    PatternCGKong(const JsonObjectConst& json) : EffectWithId<PatternCGKong>(json) { ResetGame(); }

    void Start() override { ResetGame(); }

    void ResetGame() {
        if (_resetTime == 0) {
            _resetTime = millis();
            debugEffect("ResetGame triggered. Fading out...\n");
            return;
        }
        _resetTime = 0;
        _barrels.clear();
        _mario.state = WALKING;
        _mario.x = 4.0f;
        _mario.tier = 0; // v31: Explicit tier initialization
        _mario.y = GetFloorY(4.0f, 0) - 3.0f;
        _mario.vx = kWalkSpeed;
        _mario.vy = 0;
        _mario.targetX = 64.0f;
        _mario.faceLeft = false;
        _mario.lastClimbTime = 0;
        _mario.panicTime = 0;

        _dk.x = 12.0f;
        _dk.y = 7.0f;
        _dk.frame = 0;

        _gameActive = true;
        _winTime = 0;
        _tier0Time = millis(); // v37.2: Start ground floor timer
        _barrels.clear();
        _flashAmount = 0.0f;
        debugEffect("ResetGame: Mario @ %.1f,%.1f (Tier %d). DK @ %.1f,%.1f\n", _mario.x, _mario.y, _mario.tier, _dk.x, _dk.y);
    }

    void Draw() override {
	frameTime = millis();
	_dkBreath = sinf(frameTime * 0.002f) * 0.3f;
	// 0.002f → ~3 second period (2π / 0.002 ≈ 3141 ms)
        // 0.3f → visible via Wu blending, invisible as motion

//	const CRGB dkFur     = g()->from16Bit(kDKFur565);
//	const CRGB dkTan     = g()->from16Bit(kDKTan565);
	const CRGB marioSkin = g()->from16Bit(kMarioSkin565);
	const CRGB girderCol = g()->from16Bit(kGirder565);

	// Fast, deterministic phase bits reused everywhere.
	// Shift amounts chosen to land in visually useful bands.
	const bool ditherPhase  = (frameTime >> 3) & 1;   // ~125 Hz
	const bool barrelPhase  = (frameTime >> 5) & 1;   // ~30 Hz

        // v37: Visual Fade Reset
        if (_resetTime > 0) {
            uint32_t elapsed = millis() - _resetTime;
            if (elapsed > 600) {
                ResetGame(); // Second call finishes reset
            } else {
                g()->DimAll(255 - uint8_t(elapsed * 255 / 600)); // Standard blend out
                return;
            }
        }

        g()->DimAll(160);
        if (_flashAmount > 0.1f) g()->DimAll(uint8_t(160 * (1.0f - _flashAmount))); // Pulse background
        DrawGirders();
        DrawGhostClock();

        // v16: Win Delay
        if (_winTime > 0) {
             if (millis() - _winTime > 2000) ResetGame();
        } else {
             UpdateEntities();
             UpdateBarrels();
        }

        DrawEntities();
        DrawBarrels();
        if (_flashAmount > 0) _flashAmount *= 0.92f; // Fade out flash
    }

    // Helper: Calculate Floor Y for any X and Tier
    // tier: 0=Ground, 1=T1, 2=T2, 3=T3
    float GetFloorY(float x, int tier) {
        if (tier == 0) return kGroundY; // Flat ground

        if (tier == 1) { // T1: (0, 27) -> (Max, 29)
             return 27.0f + (x / MATRIX_WIDTH) * 2.0f;
        }
        if (tier == 2) { // T2: (Max, 21) -> (0, 23)
             return 21.0f + ((MATRIX_WIDTH - x) / MATRIX_WIDTH) * 2.0f;
        }
        if (tier == 3) { // T3: (DKEdge+1, 13) -> (Max, 17)
             float dkEdge = MATRIX_WIDTH / 4.0f + 8.0f;
             if (x < dkEdge) return kDkPlatformY; // On Platform
             return 13.0f + (x - dkEdge) * (17.0f - 13.0f) / (MATRIX_WIDTH - dkEdge);
        }
        return 32.0f; // Fallback
    }

    // v33: Lookahead Helper
    bool IsAreaSafe(float x, int tier, float radius) {
        for (const auto& b : _barrels) {
            if (!b.active) continue;
            // Barrel Y is roughly FloorY - 2.0. Mario Y is FloorY - 3.0.
            // Check if barrel is on the same tier
            float floorY = GetFloorY(x, tier);

            // v35.1: Detect falling barrels near this X coordinate (Vertical Safety)
            if (b.vy > 0 && abs(b.x - x) < 3.0f) {
                 // If the barrel is "above" the target x but within 8 pixels vertically
                 if (b.y < floorY - 2.0f && b.y > floorY - 10.0f) return false;
            }

            if (abs(b.y - (floorY - 2.0f)) > 3.0f) continue;

            // Horizontal Danger
            if (abs(b.x - x) < radius) return false;
        }
        return true;
    }

    // v37.6 Ladder Safety: Check if a ladder is currently dangerous (descending OR approaching)
    bool IsLadderBusy(float ladderX, int tier) {
        for (const auto& b : _barrels) {
            if (!b.active) continue;
            // 1. Direct Danger: Vertical movement near ladder
            if (abs(b.x - ladderX) < 6.0f && b.vy != 0) return true;

            // 2. Predictive Danger: Rolling towards ladder on the same tier
            // Only relevant if barrel is on the same tier (above logic handles falling)
            // Wait, barrel on T2 rolls Left to L4(12).
            // Barrel on T1 rolls Right to L3(56).
            // Barrel on T3 rolls Right to drop.

            // Allow loose Y matching for tier check
            float floorY = GetFloorY(b.x, tier);
            if (abs(b.y - (floorY - 2.0f)) > 4.0f) continue; // Not on this tier

            // Check approach
            bool incoming = false;
            if (tier == 2 && b.x > ladderX && b.x < ladderX + 20.0f && b.vx < 0) incoming = true; // L4: From Right
            if (tier == 1 && b.x < ladderX && b.x > ladderX - 20.0f && b.vx > 0) incoming = true; // L3: From Left
            if (tier == 0 && b.x > ladderX && b.x < ladderX + 20.0f && b.vx < 0) incoming = true; // T0? Barrels roll Left.

            if (incoming) return true;
        }
        return false;
    }



private:
    void DrawGirders() {
        uint32_t girderCol = kRed;
        int dkPlatformEdge = MATRIX_WIDTH / 4 + 8;
        g()->drawLine(0, 13, dkPlatformEdge, 13, CRGB(girderCol)); // DK platform
        g()->drawLine(0, 14, dkPlatformEdge, 14, CRGB(girderCol)); // Thicken

        // Use a loop to draw 2-pixel thick slanted girders for solidity
        auto drawThickLine = [&](int x1, int y1, int x2, int y2) {
            g()->drawLine(x1, y1, x2, y2, CRGB(girderCol));
            g()->drawLine(x1, y1 + 1, x2, y2 + 1, CRGB(girderCol));
        };

        // v13: T3 slope starts at y=13 (DK Edge) down to y=17
        drawThickLine(dkPlatformEdge + 1, 13, MATRIX_WIDTH - 1, 17); // T3
        drawThickLine(MATRIX_WIDTH - 1, 21, 0, 23); // T2
        drawThickLine(0, 27, MATRIX_WIDTH - 1, 29); // T1
        g()->drawLine(0, 31, MATRIX_WIDTH - 1, 31, CRGB(girderCol)); // Ground

        DrawLadder(MATRIX_WIDTH - 8, 17, 4); // T3 to T2
        DrawLadder(8, 23, 4);               // T2 to T1
        int groundLadderX = MATRIX_WIDTH / 2;
        DrawLadder(groundLadderX, 29, 2); // T1 to Ground
    }

    void DrawLadder(int x, int y, int h) {
        for (int i = 0; i < h; i++) {
            g()->setPixel(x - 1, y + i, CRGB(CRGB::Blue));
            g()->setPixel(x + 1, y + i, CRGB(CRGB::Blue));
            if (i % 2 == 0) g()->setPixel(x, y + i, CRGB(CRGB::Blue));
        }
    }

    void DrawGhostClock() {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        struct tm tm_buf;
        struct tm* t = localtime_r(&now, &tm_buf);
        int hours = t->tm_hour;
        int mins = t->tm_min;
        int digits[5] = {hours / 10, hours % 10, 10, mins / 10, mins % 10};

        if (_winTime > 0) { // v20: Spin digits on win!
             uint8_t spin = (millis() / 50) % 10;
             for(int& d : digits) d = (d + spin) % 10;
        }
        int clockWidth = (5 * 6) + 4;
        int startX = (MATRIX_WIDTH - clockWidth) / 2;
        int startY = 10;

        CRGB ghostBase(30, 30, 30);

        for (int d = 0; d < 5; d++) {
            float highlight = 0.0f;
            int dx = startX + (d * 6);
            int dy = (d == 2) ? startY + 2 : startY;

            for (const auto& b : _barrels) {
                if (!b.active) continue;
                float distSq = powf(b.x - (dx + 3), 2) + powf(b.y - (dy + 6), 2);
                if (distSq < 100.0f) { // 10 pixel radius
                    float h = (10.0f - sqrtf(distSq)) / 10.0f;
                    highlight = std::max(highlight, h);
                }
                if (b.isBlue) highlight = std::max(highlight, 0.4f);
            }

            CRGB color = ghostBase;
            if (highlight > 0 || _flashAmount > 0) {
                float totalHighlight = std::max(highlight, _flashAmount);
                uint8_t val = (uint8_t)std::min(225.0f, 180.0f * totalHighlight + (_flashAmount > 0.1f ? 50.0f : 0.0f));
                color += CRGB(val, val, val);
            }

            DrawDigit(dx, dy, digits[d], color);
        }
    }

    void DrawDigit(int x, int y, int digit, CRGB color) {
        for (int row = 0; row < 12; row++) {
            uint16_t rowBits = font7x12[digit][row];
            for (int col = 0; col < 6; col++) {
                if ((rowBits >> (5 - col)) & 1) {
                    g()->setPixel(x + col, y + row, color);
                }
            }
        }
    }

    void UpdateDK(uint32_t nowTime) {
        // DK Pounding logic - switch between check "white static" pixels.
        if (nowTime - _dk.lastAnimTime > 400) {
            if (_dk.frame == 0) _dk.frame = 1;
            else if (_dk.frame == 1) _dk.frame = 2;
            else if (_dk.frame == 2) {
                _dk.frame = 0;
                // Spawn barrel on the pounding frame return
                // Spawn barrel on the pounding frame return
                // v24: User requested higher frequency ("bring that back"). Bump to 40%.
                // v25: Balance Pass. User felt 40% was "rain". Reduced to 20%.
                // v33.3: Balanced chaos
                if (random_range(0, 100) < 40) {
                    _barrels.push_back({25.0f, 9.5f, kClimbSpeed + (random_range(0, 100)/500.0f), 0.0f, true, false});
                }
            }
            _dk.lastAnimTime = nowTime;
        }
    }

    void UpdateMario(uint32_t nowTime) {
        // Mario AI - Seek ladders and climb
        if (_mario.state == WALKING) {
            if (nowTime - _mario.lastAnimTime > 100) {
                _mario.frame = (_mario.frame == 1) ? 2 : 1;
                _mario.lastAnimTime = nowTime;

                // Persistent target AI to prevent jitter
                // v37.10 Refactoring: Used named constants instead of local magic variables
                // int L1 = 8; int L2 = MATRIX_WIDTH / 2; int L3 = MATRIX_WIDTH - 8; (REMOVED)

                if (abs(_mario.x - _mario.targetX) < 1.0f) {
                    // Pick a new ladder target if we're not climbing
                    // _mario.targetX = (random_range(0, 100) < 50) ? L2 : (random_range(0, 100) < 50 ? L1 : L3);
                    if (_mario.tier == 0) {
                        float target = kLadder2X;
                        if (IsLadderBusy(target, 0)) {
                            _mario.targetX = (_mario.x < target) ? target - kWaitOffset : target + kWaitOffset;
                            debugEffect("Ladder L2 Busy! Waiting at %.1f\n", _mario.targetX);
                        } else {
                            _mario.targetX = target;
                        }
                    }
                    else if (_mario.tier == 1) {
                         float target = kLadder3X;
                         if (IsLadderBusy(target, 1)) {
                             float waitX = (_mario.x < target) ? target - kWaitOffsetLarge : target + kWaitOffsetLarge;
                             bool safe = IsAreaSafe(waitX, 1, kSafetyRadSmall);
                             _mario.targetX = waitX;
                             if (!safe) debugEffect("Ladder L3 Busy & Wait Spot Unsafe!\n");
                             else debugEffect("Ladder L3 Busy! Waiting at %.1f\n", waitX);
                         } else {
                             _mario.targetX = target;
                         }
                    }
                    else if (_mario.tier == 2) {
                         float target = kLadder3X;
                         if (IsLadderBusy(target, 2)) {
                             _mario.targetX = 44.0f; // Hardcoded safe spot
                              debugEffect("Ladder L3 Busy! Waiting at %.1f\n", _mario.targetX);
                         } else {
                             _mario.targetX = target;
                         }
                    }
                    else if (_mario.tier == 3) {
                         _mario.targetX = 0.0f; // Run to DK
                    }
                }

                if (_mario.vx > 0) _mario.faceLeft = false;
                else if (_mario.vx < 0) _mario.faceLeft = true;

                _mario.vx = (_mario.x < _mario.targetX) ? kWalkSpeed : -kWalkSpeed;
                _mario.x += _mario.vx;

                // v16.1 Pants Logic: Draw uses CENTERED y (y+r-4).
                // Feet are row 7. y_feet = y + 7 - 4 = y + 3.
                // We want Feet at FloorY - 1 (Standing ON line).
                // y + 3 = FloorY - 1  =>  y = FloorY - 4.

                // v17/18 Thresholds: Uses named constants kThreshG/T1/T2

                float feetY = _mario.y + 3.0f;
                debugEffect("Mario: x%.1f y%.1f Feet: %.1f Tier: %d\n", _mario.x, _mario.y, feetY, _mario.tier);

                // v30: Deterministic snapping based on tier variable
                _mario.y = GetFloorY(_mario.x, _mario.tier) - 3.0f;

                if (_mario.tier == 3) {
                     // WIN CONDITION: Approach DK (Target x < 14)
                     float dkEdge = MATRIX_WIDTH / 4.0f + 8.0f;
                      if (_mario.x < dkEdge - 2.0f) {
                          debugEffect("Mario Reached DK! Win!\n");
                          _flashAmount = 1.0f; // Flash!
                          _winTime = millis(); // Trigger delay
                          return;
                      }
                }

                // Count barrels for strategic awareness
                auto countBarrels = [&](int t) {
                    int c = 0;
                    for (const auto& b : _barrels) if (b.active && abs(b.y - (GetFloorY(b.x, t) - 2.0f)) < 3.0f) c++;
                    return c;
                };
                bool crowdLevel = countBarrels(_mario.tier) >= 2;
                bool crowdAbove = countBarrels(_mario.tier + 1) >= 2;

                    // v37.1: Tier-aware Bounding Box Collision
                    // Mario spans [y-4, y+3]. Barrel spans [y-1, y+1].
                    // v37.5: Reduced box height to [y-3, y+3] to prevent headshots from falling barrels
                    for (const auto& b : _barrels) {
                    if (!b.active) continue;
                    float dx = b.x - _mario.x;
                    float dy = b.y - _mario.y; // b.y - m.y

                    // Same-floor collision: dy should be ~1.0
                    bool sameFloor = (dy > -1.0f && dy < 2.5f);
                    // Falling collision: catches barrels between floors (gap is 4-6px)
                    // v37.5: Loosened from dy > -4.0f to dy > -3.5f for "Headshot" grace
                    // v37.8: Tightened to dy > -3.0f to be even more forgiving of head-grazes
                    // v37.9: TIGHTENED FURTHER to dy > -2.0f based on 120s log analysis (4 headshots at -2.2)
                    bool fallingHit = (dy > -2.0f && dy <= -1.0f);

                    if (abs(dx) < 3.2f && (sameFloor || fallingHit)) {
                         if (_mario.state != JUMPING) {
                             debugEffect("Mario Died! Collision (dx:%.1f dy:%.1f Floor:%d Fall:%d)\n", dx, dy, sameFloor, fallingHit);
                             ResetGame();
                             return;
                         }
                    }

                    if (_mario.state == WALKING && abs(dy - 2.0f) < 3.0f) {
                        // Multi-barrel detection (v35: 30px shield)
                        bool b2close = false;
                        for (auto& b2 : _barrels) {
                            if (&b2 == &b) continue;
                            if (abs(b2.y - _mario.y) > 2.0f) continue;
                            float d2 = b2.x - _mario.x;
                            if (abs(d2) < 30.0f) {
                                b2close = true;
                                break;
                            }
                        }

                        // JUMP TRIGGER - Refuse if sandwich detected
                        // v36: Desperation Jump if trapped near walls (x < 8 or x > width-8)
                        bool trapped = (_mario.x < 8.0f || _mario.x > MATRIX_WIDTH - 8.0f);
                        bool inPanic = (nowTime - _mario.panicTime < 1000);
                        float jumpDist = (trapped && inPanic) ? 10.0f : 7.0f;

                        // v37.9: Sandwich Panic Boost!
                        // If sandwiched, we MUST jump earlier (shield is 30px, jump at 15px).
                        // Otherwise we wait until 7px and get crushed.
                        if (b2close) jumpDist = 15.0f;

                        if (abs(dx) < jumpDist) {
                            // v37.4 Smart Jump Timing: Refuse "Bad Hops"
                            // If barrel is high up (falling/ladder) or still far away, defer jump to avoid landing on it
                            bool barrelFalling = (b.vy != 0);
                            // v37.8: Force Jump if Sandwiched (b2close) to escape crushing
                            bool almostDead = (abs(dx) < 7.0f) || b2close;

                            // v37.8: Check Safety of Landing Zone (Ladder Bases are dangerous!)
                            float landingX = _mario.x + (_mario.vx * 15.0f);
                            bool landingOnLadder = IsLadderBusy(landingX, _mario.tier); // Check if landing on a busy ladder

                            if (barrelFalling && abs(dx) > 10.0f) {
                                debugEffect("Jump Deferred: Barrel too high/far (dx:%.1f)\n", dx);
                            } else if (!almostDead && !IsAreaSafe(landingX, _mario.tier, 6.0f)) { // Check landing zone roughly
                                debugEffect("Jump Deferred: Landing Unsafe!\n");
                            } else if (!almostDead && landingOnLadder) { // v37.8: Don't jump into a fire
                                debugEffect("Jump Deferred: Landing on Busy Ladder!\n");
                            } else if (b2close && !almostDead) {
                                // v37.8: Replaced by 'almostDead' override above.
                                // If we get here, it means we are NOT almostDead (so not sandwiched close enough?)
                                // Actually, b2close sets almostDead=true, so we skip this.
                                // Logic preserved for historical diffs if we revert.
                                debugEffect("Jump Refused! Sandwich at dx:%.1f\n", dx);
                            } else {
                                if (b2close) debugEffect("Desperation Jump! SANDWICH ESCAPE!\n");
                                else if (abs(dx) > 7.0f) debugEffect("Desperation Jump! Trapped at x:%.1f\n", _mario.x);

                                _mario.state = JUMPING;
                                _mario.vy = kJumpStrength;
                                _mario.jumpFloorY = _mario.y;
                                // v37: Horizontal Jump Kick (Ensure momentum)
                                if (abs(_mario.vx) < 0.2f) {
                                    float kick = (_mario.x < MATRIX_WIDTH/2) ? 0.4f : -0.4f;
                                    _mario.vx = (abs(dx) < 1.0f) ? kick : (_mario.vx = (dx > 0 ? -0.4f : 0.4f));
                                }
                                debugEffect("Mario jumped! dx:%.1f vx:%.2f\n", dx, _mario.vx);
                                break;
                            }
                        }

                        // EVASION (Only for clusters)
                        // v37.2: Tier-specific panic sensitivity
                        bool retreating = (_mario.vx * dx < 0);
                        if (b2close) {
                             float baseDist = (_mario.tier == 0) ? 15.0f : 20.0f; // Braver on ground
                             float safeDist = retreating ? 30.0f : baseDist;
                             if (abs(dx) < safeDist) {
                                  debugEffect("Cluster Panic! dx:%.1f (T%d)\n", dx, _mario.tier);
                                  _mario.x -= _mario.vx * 2.5f;
                                  _mario.vx = 0;
                                  _mario.panicTime = nowTime;
                             }
                        }
                    }
                }

                // PERSISTENT RETREAT/PAUSE (v37.2: Tier-specific panic duration)
                uint32_t panicDuration = (_mario.tier == 0) ? 700 : 1000; // Faster recovery on ground
                if (nowTime - _mario.panicTime < panicDuration) {
                     // Active backpedal away from target (v36: 1.2f, STOP at wall)
                     bool atWall = (_mario.x < 5.0f || _mario.x > MATRIX_WIDTH - 5.0f);
                     float retreatDir = (_mario.x < _mario.targetX) ? -1.2f : 1.2f;

                     // v37.4: Sandwich Evasion (Heel Check)
                     // Before running backward, check if we are running into another barrel!
                     if (!IsAreaSafe(_mario.x + (retreatDir * 10.0f), _mario.tier, 6.0f)) {
                         _mario.vx = 0; // Hold Ground!
                         debugEffect("Sandwich! Holding ground. (Heel Check)\n");
                     } else {
                         _mario.vx = atWall ? 0 : retreatDir;
                     }
                     _mario.faceLeft = (_mario.x < _mario.targetX); // Face danger while retreating
                } else {
                     // Always update walk direction towards target (v35.1: fixed logic)
                     float dir = (_mario.x < _mario.targetX) ? 1.0f : -1.0f;
                     if (abs(_mario.vx) > kWalkSpeed || _mario.vx == 0) {
                          debugEffect("Panic cleared, resuming walk.\n");
                     }
                     _mario.vx = dir * kWalkSpeed;
                     _mario.faceLeft = (_mario.vx < 0);
                }
                _mario.x += _mario.vx;

                // Check ladder proximity (v34.5: Widen detection for bailing)
                // v37.10: Use constants
                bool nearL1 = abs(_mario.x - kLadder1X) < kLadderProximity;
                bool nearL2 = abs(_mario.x - kLadder2X) < kLadderProximity;
                bool nearL3 = abs(_mario.x - kLadder3X) < kLadderProximity;
                bool bailL1 = abs(_mario.x - kLadder1X) < kBailProximity;
                bool bailL2 = abs(_mario.x - kLadder2X) < kBailProximity;
                bool bailL3 = abs(_mario.x - kLadder3X) < kBailProximity;
                bool climbCooldown = (nowTime - _mario.lastClimbTime < 1500);

                // Helper to start climb
                auto startClimb = [&](float ladderX, float targetY) {
                    _mario.state = CLIMBING;
                    _mario.x = ladderX; // SNAP to ladder center
                    _mario.vy = (targetY < _mario.y) ? -kClimbSpeed : kClimbSpeed; // UP or DOWN
                    _mario.vx = 0;
                    _mario.climbTargetY = targetY;

                    // v28: Smart Target Logic (v37.10: Refactored constants)
                    if (targetY < _mario.y) { // UP
                        if (abs(ladderX - kLadder2X) < kLadderSnapDist) _mario.targetX = kLadder1X;
                        else if (abs(ladderX - kLadder1X) < kLadderSnapDist) _mario.targetX = kLadder3X;
                        else if (abs(ladderX - kLadder3X) < kLadderSnapDist) _mario.targetX = kLadder4X;
                    } else { // DOWN
                        if (abs(ladderX - kLadder3X) < kLadderSnapDist) _mario.targetX = kLadder1X;
                        else if (abs(ladderX - kLadder1X) < kLadderSnapDist) _mario.targetX = kLadder2X;
                        else _mario.targetX = (random_range(0,100)<50) ? kLadder1X : kLadder3X;
                    }
                    debugEffect("Mario climbing %.1f to %.1f. Next Target: %.1f\n", ladderX, targetY, _mario.targetX);
                };

                if (!climbCooldown) {
                    auto checkL = [&](float lx, int targetT, float rBase, float rDest, const char* label) {
                         bool safeDest = IsAreaSafe(lx, targetT, rDest);
                         bool safeBase = IsAreaSafe(lx, _mario.tier, rBase);

                         // v37.2: Tier-specific Safe Waiting Zone
                         bool waiting = false;
                         if (!safeDest || !safeBase) {
                             float distToLadder = abs(_mario.x - lx);
                             // Tighter wait zone on Tier 0 for L2 to capitalize on gaps
                             float waitDist = (_mario.tier == 0 && abs(lx - kLadder2X) < kLadderSnapDist) ? kSafetyRadSmall : kSafetyRadLarge;
                             if (distToLadder < waitDist) {
                                 float offset = (lx < MATRIX_WIDTH/2) ? waitDist : -waitDist;
                                 _mario.targetX = lx + offset;
                                 waiting = true;
                             }
                         }

                         if (!safeDest) debugEffect("Ladder %s UNSAFE at Dest. Waiting:%d\n", label, waiting);
                         if (!safeBase) debugEffect("Ladder %s UNSAFE at Base. Waiting:%d\n", label, waiting);
                         return safeDest && safeBase;
                    };

                    // v37: Correct danger-behind detection based on tier roll direction
                    bool barrelsMoveRight = (_mario.tier % 2 != 0); // T1, T3 move Right (from Left)
                    float dangerXOffset = barrelsMoveRight ? -15.0f : 15.0f;
                    bool dangerBehind = !IsAreaSafe(_mario.x + dangerXOffset, _mario.tier, 12.0f);

                    // v37.2: Desperation climb if stuck on Tier 0 for >15 seconds
                    bool desperateClimb = (_mario.tier == 0 && (nowTime - _tier0Time) > 15000);
                    if (desperateClimb) debugEffect("Desperation Climb! T0 time: %.1fs\n", (nowTime - _tier0Time) / 1000.0f);

                    // v34.1: Correctly split LX proximity checks
                    if (_mario.tier == 0) {
                        if (nearL2 && (desperateClimb || checkL(kLadder2X, 1, kSafetyRadSmall, kSafetyRadLarge, "L2")) && !crowdAbove) {
                            if (_mario.targetX == kLadder2X || random_range(0, 100) < 80 || dangerBehind || desperateClimb) startClimb(kLadder2X, 25.0f);
                        }
                    } else if (_mario.tier == 1) {
                        if (nearL1 && checkL(kLadder1X, 2, kSafetyRadSmall, kSafetyRadHuge, "L1") && !crowdAbove) {
                            if (_mario.targetX == kLadder1X || random_range(0, 100) < 80 || dangerBehind) startClimb(kLadder1X, 19.5f);
                        }
                        if (bailL2 && (random_range(0,100) < 60 || crowdLevel) && IsAreaSafe(kLadder2X, 0, kWaitOffset)) {
                            debugEffect("Mario bailing to T0! Crowd:%d\n", (int)crowdLevel);
                            startClimb(kLadder2X, 31.0f);
                        }
                    } else if (_mario.tier == 2) {
                        if (nearL3 && checkL(kLadder3X, 3, kSafetyRadSmall, kSafetyRadLarge, "L3") && !crowdAbove) {
                            if (_mario.targetX == kLadder3X || random_range(0, 100) < 80) startClimb(kLadder3X, 13.0f);
                        }
                        if (bailL1 && (random_range(0,100) < 60 || crowdLevel) && IsAreaSafe(kLadder1X, 1, kWaitOffset)) {
                            // v37.7: Check if L1 is safe to descend!
                            if (!IsLadderBusy(kLadder1X, 1)) { // Check destination tier (1) or current? Descending into 1.
                                debugEffect("Mario bailing to T1! Crowd:%d\n", (int)crowdLevel);
                                startClimb(kLadder1X, 26.0f);
                            } else {
                                // Stuck near L1 and it's busy? Wait LEFT (x=4) if safe.
                                if (IsAreaSafe(4.0f, 2, kSafetyRadSmall)) _mario.targetX = 4.0f;
                            }
                        }
                    } else if (_mario.tier == 3) {
                         if (bailL3 && (random_range(0,100) < 60 || crowdLevel) && IsAreaSafe(kLadder3X, 2, kSafetyRadLarge)) {
                             debugEffect("Mario bailing to T2! Crowd:%d\n", (int)crowdLevel);
                             startClimb(kLadder3X, 19.5f);
                         }
                    }
                }

                if (_mario.x > MATRIX_WIDTH - 4 || _mario.x < 4) _mario.vx *= -1.0f;
            }
        } else if (_mario.state == CLIMBING) {
            _mario.y += _mario.vy;
            _mario.frame = 3; // Climbing frame

            if (_mario.vy < 0) { // UP
                if (_mario.y <= _mario.climbTargetY) {
                    _mario.state = WALKING;
                    _mario.y = _mario.climbTargetY;
                    _mario.vy = 0;
                    int prevTier = _mario.tier;
                    _mario.tier++; // v30: Moved UP a tier
                    if (prevTier == 0) _tier0Time = millis(); // v37.2: Reset ground floor timer
                    _mario.lastClimbTime = millis(); // v32: Set cooldown
                    _mario.vx = (_mario.x < _mario.targetX) ? kWalkSpeed : -kWalkSpeed;
                    debugEffect("Mario arrived at target %.1f (Tier %d)\n", _mario.climbTargetY, _mario.tier);
                }
            } else if (_mario.vy > 0) { // DOWN (v27 Fix)
                if (_mario.y >= _mario.climbTargetY) {
                    _mario.state = WALKING;
                    _mario.y = _mario.climbTargetY;
                    _mario.vy = 0;
                    _mario.tier--; // v30: Moved DOWN a tier
                    _mario.lastClimbTime = millis(); // v32: Set cooldown
                    _mario.vx = (_mario.x < _mario.targetX) ? kWalkSpeed : -kWalkSpeed;
                    debugEffect("Mario arrived (down) at target %.1f (Tier %d)\n", _mario.climbTargetY, _mario.tier);
                }
            }
        } else if (_mario.state == JUMPING) {
            _mario.x += _mario.vx;
            _mario.y += _mario.vy;
            _mario.vy += kGravity; // Gravity

            // Land on floor (Snap to jumpFloorY)
            if (_mario.y >= _mario.jumpFloorY && _mario.vy > 0) {
                 _mario.y = _mario.jumpFloorY; _mario.vy = 0; _mario.state = WALKING;
            }
        }

        // v24: Negative X Check (Safety Clamp)
        if (_mario.x < 4.0f) _mario.x = 4.0f;
        if (_mario.x > MATRIX_WIDTH + 4.0f) _mario.x = MATRIX_WIDTH + 4.0f; // Allow off-screen win, but limit garbage
    }

    void UpdateEntities() {
        uint32_t nowTime = millis();
        UpdateDK(nowTime);
        UpdateMario(nowTime);
    }

    void DrawEntities() {
        // DK - Draw palette layers
        for (int r = 0; r < 12; r++) {
            uint16_t furBits = dkFur[_dk.frame][r];
            // uint16_t tanBits = dkTan[_dk.frame][r];
            uint16_t dkTanBits = dkTan[_dk.frame][r];
            for (int c = 0; c < 12; c++) {
// FIXME: Change these CRGBs to dkSkin and dkFur
                if ((dkTanBits >> (11 - c)) & 1) g()->drawPixelXYF_Wu(_dk.x + c - 6, _dk.y + r - 6 + _dkBreath,  CRGB(0xCC9977)); // Tan skin
                else if ((furBits >> (11 - c)) & 1) g()->drawPixelXYF_Wu(_dk.x + c - 6, _dk.y + r - 6 + _dkBreath, CRGB(0x331100)); // Darker Brown
            }
        }
        // Mario - Draw multi-color layers
        CRGB skinCol(0xFFAAAA);
        for (int r = 0; r < 8; r++) {
            uint8_t rBits = marioRed[_mario.frame][r];
            uint8_t bBits = marioBlue[_mario.frame][r];
            uint8_t sBits = marioSkin[_mario.frame][r];
            for (int c = 0; c < 8; c++) {
                CRGB col = CRGB::Black;
                // Sprite Source assumed LEFT-FACING based on user feedback.
                // If vx > 0 (Right), faceLeft=false. We want RIGHT. If source is LEFT, we must FLIP (7-c).
                // If vx < 0 (Left), faceLeft=true. We want LEFT. If source is LEFT, we use NORMAL (c).
                int colIdx = _mario.faceLeft ? c : (7 - c);
                if ((rBits >> (7 - colIdx)) & 1) col = CRGB::Red;
                else if ((bBits >> (7 - colIdx)) & 1) col = CRGB::Blue;
                else if ((sBits >> (7 - colIdx)) & 1) col = skinCol;

                if (col != CRGB::Black) g()->drawPixelXYF_Wu(_mario.x + c - 4, _mario.y + r - 4 + marioSquash, col);
            }
        }
    }

    void UpdateBarrels() {
        for (auto& b : _barrels) {
            if (!b.active) continue;
            if (!b.active) continue;
            // v22: Verbose Logging as requested
            debugEffect("B: x%.1f y%.1f vx%.1f vy%.1f\n", b.x, b.y, b.vx, b.vy);

            b.x += b.vx;
            b.y += b.vy;

            // v14 Physics: Use GetFloorY helpers for slope targets
            // To roll ON top of line: Target Y = FloorY - 2.0f. (Bottom row at FloorY-1, Line at FloorY)

            // Current Tier Logic (simplified vs FSM for brevity, but kept structure)
            if (b.y < kT3Y + 1.0f && b.vx > 0) { // T3 Slope
                 b.y = GetFloorY(b.x, 3) - 2.0f;
                 if (b.x >= MATRIX_WIDTH - 8) { b.x = MATRIX_WIDTH - 8; b.vx = 0; b.vy = 0.5f; }
            }
            else if (b.vx == 0 && b.vy > 0 && b.y < 22) { // Ladder T3->T2
                 // v23: Add X-bound (>32) to prevent catching L1 drops (x=8)!
                 if (b.x > 32 && b.y >= 20.0f) { b.y = 20.0f; b.vx = -0.5f; b.vy = 0; }
            }
            else if (b.vx < 0 && b.y < 24) { // T2 Slope
                 b.y = GetFloorY(b.x, 2) - 2.0f;
                 if (b.x <= 10) {
                     b.x = 8.0f; b.vx = 0; b.vy = 0.5f;
                     // debugEffect("Barrel hitting L1 descent! Snap to 8.\n"); // v19: Silenced
                 }
            }
            else if (b.vx == 0 && b.vy > 0 && b.y < 28) { // Ladder T2->T1
                 // Target T1 Top. v20: Add X Check to prevent catching T1->Gnd drops!
                 if (b.x < 16 && b.y >= 26.0f) { b.y = 26.0f; b.vx = 0.5f; b.vy = 0; }
            }
            else if (b.vx > 0 && b.y < 30) { // T1 Slope
                 b.y = GetFloorY(b.x, 1) - 2.0f;
                 if (b.x >= MATRIX_WIDTH - 16) { b.x = MATRIX_WIDTH - 16; b.vx = 0; b.vy = 0.5f; }
            }
            else if (b.vx == 0 && b.vy > 0 && b.y >= kT1Y + 0.5f) { // Ladder T1->Ground
                 if (b.x > 32 && b.y >= 30.0f) {
                     b.y = 30.0f;
                     b.vx = -0.6f; // v15: Always roll LEFT on ground to use full floor
                     b.vy = 0;
                 }
            } else if (b.y >= 30.0f) { // Ground Roll
                 b.y = GetFloorY(b.x, 0) - 2.0f;
                 if (b.x < 2) b.active = false; // Despawn at left edge
            }

            if (b.x < -10 || b.x > MATRIX_WIDTH + 10) b.active = false;
        }
    }

    void DrawBarrels() {
	const CRGB barrelCol = g()->from16Bit(kBarrel565);
// 	const int phase = barrelPhase ? 1 : 0;
	const bool barrelPhase  = (frameTime >> 5) & 1;   // ~30 Hz

        for (const auto& b : _barrels) {
            if (!b.active) continue;
            // Draw 4x3 bitmap centered on b.x, b.y
            // 4x3 bitmap:
            // .XX.
            // XXXX
            // .XX.
#if 1
	    CRGB palBarrel = g()->from16Bit(kBarrel565);
	    // Barrel color (blue or gamma-corrected brown)
	    CRGB col = b.isBlue ? CRGB::Blue : palBarrel;

	    // Simple rolling phase: offset rows to simulate rotation
	    int barrelPhase = (frameTime >> 5) & 1;
barrelPhase = 0;

	    // Row 0
	    g()->drawPixelXYF_Wu(b.x - (1 + barrelPhase), b.y - 1, col);
	    g()->drawPixelXYF_Wu(b.x      + barrelPhase,  b.y - 1, col);

	    // Row 1
	    g()->drawPixelXYF_Wu(b.x - 2, b.y, col);
	    g()->drawPixelXYF_Wu(b.x - 1, b.y, col);
	    g()->drawPixelXYF_Wu(b.x    , b.y, col);
	    g()->drawPixelXYF_Wu(b.x + 1, b.y, col);

	    // Row 2
	    g()->drawPixelXYF_Wu(b.x - (1 + (1 - barrelPhase)), b.y + 1, col);
	    g()->drawPixelXYF_Wu(b.x      + (1 - barrelPhase),  b.y + 1, col);

#else
            CRGB col = b.isBlue ? CRGB::Blue : barrelCol;

            // Row 0 (.XX.)
            g()->drawPixelXYF_Wu(b.x - 1, b.y - 1, col);
            g()->drawPixelXYF_Wu(b.x,     b.y - 1, col);
            // Row 1 (XXXX)
            g()->drawPixelXYF_Wu(b.x - 2, b.y,     col);
            g()->drawPixelXYF_Wu(b.x - 1, b.y,     col);
            g()->drawPixelXYF_Wu(b.x,     b.y,     col);
            g()->drawPixelXYF_Wu(b.x + 1, b.y,     col);
            // Row 2 (.XX.)
            g()->drawPixelXYF_Wu(b.x - 1, b.y + 1, col);
            g()->drawPixelXYF_Wu(b.x,     b.y + 1, col);
#endif
        }
    }
};

#endif // PATTERN_CG_KONG_H
