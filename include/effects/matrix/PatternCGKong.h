#ifndef PATTERN_CG_KONG_H
#define PATTERN_CG_KONG_H

#include "globals.h"
#include <vector>
#include <chrono>
#include <algorithm>
#include <cmath>


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
    };

    std::vector<Barrel> _barrels;
    Entity _dk;
    Entity _mario;
    int _lastMin = -1;
    float _flashAmount = 0.0f;
    bool _gameActive = true;
    uint32_t _winTime = 0; // v16: Time of victory

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

public:
    PatternCGKong() : EffectWithId<PatternCGKong>("CG Kong") { ResetGame(); }
    PatternCGKong(const JsonObjectConst& json) : EffectWithId<PatternCGKong>(json) { ResetGame(); }

    void Start() override { ResetGame(); }

    void ResetGame() {
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
        
        _dk.x = 12.0f;
        _dk.y = 7.0f; 
        _dk.frame = 0;
        
        _gameActive = true;
        _winTime = 0;
        _flashAmount = 0.0f;
        debugA("ResetGame: Mario @ %.1f,%.1f (Tier 0). DK @ 12.0,7.0\n", _mario.x, _mario.y);
    }

    void Draw() override {
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
            if (abs(b.y - (floorY - 2.0f)) > 3.0f) continue; 
            
            if (abs(b.x - x) < radius) return false;
        }
        return true;
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
                // v32: Kong-sized challenge!
                if (random_range(0, 100) < 60) { 
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
                int L1 = 8;
                int L2 = MATRIX_WIDTH / 2;
                int L3 = MATRIX_WIDTH - 8;

                if (abs(_mario.x - _mario.targetX) < 1.0f) {
                    // Pick a new ladder target if we're not climbing
                    _mario.targetX = (random_range(0, 100) < 50) ? L2 : (random_range(0, 100) < 50 ? L1 : L3);
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
                debugA("Mario: x%.1f y%.1f Feet: %.1f Tier: %d\n", _mario.x, _mario.y, feetY, _mario.tier);

                // v30: Deterministic snapping based on tier variable
                _mario.y = GetFloorY(_mario.x, _mario.tier) - 3.0f;

                if (_mario.tier == 3) {
                     // WIN CONDITION: Approach DK (Target x < 14)
                     float dkEdge = MATRIX_WIDTH / 4.0f + 8.0f;
                      if (_mario.x < dkEdge - 2.0f) {
                          debugA("Mario Reached DK! Win!\n");
                          _flashAmount = 1.0f; // Flash!
                          _winTime = millis(); // Trigger delay
                          return;
                      }
                }

                // v32: Global Collision (Mario is vulnerable while walking/climbing)
                for (const auto& b : _barrels) {
                    if (!b.active) continue;
                    float dx = b.x - _mario.x;
                    float dy = b.y - _mario.y;
                    
                    if (abs(dx) < 3.0f && abs(dy - 2.0f) < 2.0f) { 
                         if (_mario.state != JUMPING) {
                             debugA("Mario Died! (dx:%.1f dy:%.1f State:%d)\n", dx, dy, _mario.state);
                             ResetGame();
                             return;
                         }
                    }

                    if (_mario.state == WALKING && abs(dy - 2.0f) < 3.0f) {
                        // v33: Smarter JUMP (Only if necessary or safe)
                        // If there's another barrel closely behind this one, JUMPING is suicide.
                        bool multiBarrel = false;
                        for (const auto& b2 : _barrels) {
                            if (&b == &b2 || !b2.active) continue;
                            float dx2 = b2.x - _mario.x;
                            // If B2 is behind B1 (relative to Mario's jump trajectory)
                            if (abs(dx2) > abs(dx) && abs(dx2) < 18.0f && abs(b2.y - b.y) < 2.0f) {
                                multiBarrel = true;
                                break;
                            }
                        }

                        // JUMP TRIGGER (Close range: < 7px) - v33: Refuse jump if multibarrel
                        if (abs(dx) < 7.0f && !multiBarrel) {
                            _mario.state = JUMPING; 
                            _mario.vy = kJumpStrength;
                            _mario.jumpFloorY = _mario.y;
                            if (abs(_mario.vx) < 0.1f) {
                                _mario.vx = (_mario.x < _mario.targetX) ? kWalkSpeed : -kWalkSpeed;
                            }
                            debugA("Mario jumped! dx:%.1f multi:%d\n", dx, multiBarrel);
                            break; 
                        }

                        // EVASION (Wider range: 15px)
                        bool retreating = (_mario.vx < 0 && dx > 0) || (_mario.vx > 0 && dx < 0);
                        float safeDist = (retreating || multiBarrel) ? 18.0f : 12.0f;
                        if (abs(dx) < safeDist) {
                             _mario.x -= _mario.vx * 1.5f; // Backpedal
                             _mario.vx = 0; 
                        }
                    }
                }

                // Check ladder proximity (v18 Widen to < 4.0f)
                bool nearL1 = abs(_mario.x - L1) < 4.0f;
                bool nearL2 = abs(_mario.x - L2) < 4.0f;
                bool nearL3 = abs(_mario.x - L3) < 4.0f;
                bool climbCooldown = (nowTime - _mario.lastClimbTime < 1500);

                // Helper to start climb
                auto startClimb = [&](float ladderX, float targetY) {
                    _mario.state = CLIMBING; 
                    _mario.x = ladderX; // SNAP to ladder center
                    _mario.vy = (targetY < _mario.y) ? -kClimbSpeed : kClimbSpeed; // UP or DOWN
                    _mario.vx = 0;
                    _mario.climbTargetY = targetY; 
                    
                    // v28: Smart Target Logic
                    if (targetY < _mario.y) { // UP
                        if (abs(ladderX - L2) < 2.0f) _mario.targetX = L1;
                        else if (abs(ladderX - L1) < 2.0f) _mario.targetX = L3;
                        else if (abs(ladderX - L3) < 2.0f) _mario.targetX = 12.0f;
                    } else { // DOWN
                        if (abs(ladderX - L3) < 2.0f) _mario.targetX = L1;
                        else if (abs(ladderX - L1) < 2.0f) _mario.targetX = L2;
                        else _mario.targetX = (random_range(0,100)<50) ? 8.0f : 56.0f;
                    }
                    debugA("Mario climbing %.1f to %.1f. Next Target: %.1f\n", ladderX, targetY, _mario.targetX);
                };

                if (!climbCooldown) {
                    if (nearL2 && _mario.y > kThreshG) { // Ground to T1
                        if (IsAreaSafe(L2, 1, 8.0f)) { // v33: Lookahead!
                            if (_mario.targetX == L2 || random_range(0, 100) < 80) startClimb(L2, 25.0f); 
                        }
                    } else if (nearL1 && _mario.y > kT2Y && _mario.y < kT1Y) { // T1 to T2
                        if (IsAreaSafe(L1, 2, 8.0f)) {
                            if (_mario.targetX == L1 || random_range(0, 100) < 80) startClimb(L1, 19.5f);
                        }
                        if (abs(_mario.x - L2) < 4.0f && random_range(0,100) < 20 && IsAreaSafe(L2, 0, 8.0f)) startClimb(L2, 31.0f);
                    } else if (nearL3 && _mario.y > kT3Y && _mario.y < kT2Y) { // T2 to T3
                        if (IsAreaSafe(L3, 3, 8.0f)) {
                            if (_mario.targetX == L3 || random_range(0, 100) < 80) startClimb(L3, 13.0f);
                        }
                        if (abs(_mario.x - L1) < 4.0f && random_range(0,100) < 20 && IsAreaSafe(L1, 1, 8.0f)) startClimb(L1, 26.0f);
                    } else if (nearL3 && _mario.tier == 3) { // T3 (Top)
                         if (random_range(0,100) < 20 && IsAreaSafe(L3, 2, 8.0f)) startClimb(L3, 19.5f); 
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
                    _mario.tier++; // v30: Moved UP a tier
                    _mario.lastClimbTime = millis(); // v32: Set cooldown
                    _mario.vx = (_mario.x < _mario.targetX) ? kWalkSpeed : -kWalkSpeed;
                    debugA("Mario arrived at target %.1f (Tier %d)\n", _mario.climbTargetY, _mario.tier);
                }
            } else if (_mario.vy > 0) { // DOWN (v27 Fix)
                if (_mario.y >= _mario.climbTargetY) {
                    _mario.state = WALKING;
                    _mario.y = _mario.climbTargetY;
                    _mario.vy = 0;
                    _mario.tier--; // v30: Moved DOWN a tier
                    _mario.lastClimbTime = millis(); // v32: Set cooldown
                    _mario.vx = (_mario.x < _mario.targetX) ? kWalkSpeed : -kWalkSpeed;
                    debugA("Mario arrived (down) at target %.1f (Tier %d)\n", _mario.climbTargetY, _mario.tier);
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
            uint16_t tanBits = dkTan[_dk.frame][r];
            for (int c = 0; c < 12; c++) {
                if ((tanBits >> (11 - c)) & 1) g()->drawPixelXYF_Wu(_dk.x + c - 6, _dk.y + r - 6, CRGB(0xCC9977)); // Tan skin
                else if ((furBits >> (11 - c)) & 1) g()->drawPixelXYF_Wu(_dk.x + c - 6, _dk.y + r - 6, CRGB(0x331100)); // Darker Brown
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
                
                if (col != CRGB::Black) g()->drawPixelXYF_Wu(_mario.x + c - 4, _mario.y + r - 4, col);
            }
        }
    }

    void UpdateBarrels() {
        for (auto& b : _barrels) {
            if (!b.active) continue;
            if (!b.active) continue;
            // v22: Verbose Logging as requested
            debugA("B: x%.1f y%.1f vx%.1f vy%.1f\n", b.x, b.y, b.vx, b.vy);

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
                     // debugA("Barrel hitting L1 descent! Snap to 8.\n"); // v19: Silenced
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
        for (const auto& b : _barrels) {
            if (!b.active) continue;
            // Draw 4x3 bitmap centered on b.x, b.y
            // 4x3 bitmap:
            // .XX.
            // XXXX
            // .XX.
            CRGB col = b.isBlue ? CRGB::Blue : CRGB(0x994400);
            
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
        }
    }
};

#endif // PATTERN_CG_KONG_H
