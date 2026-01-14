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

    enum EntityState { WALKING, CLIMBING, JUMPING };
    struct Entity {
        float x, y;
        uint8_t frame;
        uint32_t lastAnimTime;
        EntityState state = WALKING;
        float vx = 0;
        float vy = 0;
        float targetX = 0;
        bool faceLeft = false;
        float jumpFloorY = 28.0f;
        float climbTargetY = 0.0f; // New: Explicit target to stop climbing "humps"
    };

    std::vector<Barrel> _barrels;
    Entity _dk;
    Entity _mario;
    int _lastMin = -1;
    float _flashAmount = 0.0f;

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
        {0x000, 0x0E0, 0x1F0, 0x3F8, 0x7FC, 0x7FC, 0x7FC, 0xFFF, 0xFFF, 0x7FC, 0x3F8, 0x228}, // Standing (hunch) -- unchanged
        {0x000, 0x0E0, 0x1F0, 0x7FC, 0xFFF, 0xC3F, 0xC3F, 0xC3F, 0xFFF, 0x7FC, 0x666, 0x000}, // Pounding A (Arms out, gaps at sides)
        {0x7FC, 0xFFF, 0xC3F, 0xC3F, 0xC3F, 0xFFF, 0x7FC, 0x7FC, 0x7FC, 0x3F8, 0x228, 0x000}  // Pounding B (Arms up, gaps at sides)
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
        // DK sitting ON platform (y=13). Center at y=7 (Bottom at 12).
        _dk = {12.0f, 7.0f, 0, 0};
        // Mario sitting ON ground (y=31 -> y=32 for depth). Center at y=28.
        // Start him at x=10 (safe spot) instead of 32 (Ladder).
        _mario = {10.0f, 28.0f, 0, 0};
        _mario.targetX = 8.0f; // Initial target
        _lastMin = -1;
    }

    void Draw() override {
        g()->DimAll(160);
        if (_flashAmount > 0.1f) g()->DimAll(uint8_t(160 * (1.0f - _flashAmount))); // Pulse background
        DrawGirders();
        DrawGhostClock();
        UpdateEntities();
        DrawEntities();
        UpdateBarrels();
        DrawBarrels();
        if (_flashAmount > 0) _flashAmount *= 0.92f; // Fade out flash
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

        drawThickLine(dkPlatformEdge + 1, 15, MATRIX_WIDTH - 1, 17); // T3
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

    void UpdateEntities() {
        uint32_t nowTime = millis();
        // DK Pounding logic - switch between Stand (0), Arms out (1), Pounding (2)
        if (nowTime - _dk.lastAnimTime > 400) {
            if (_dk.frame == 0) _dk.frame = 1;
            else if (_dk.frame == 1) _dk.frame = 2;
            else if (_dk.frame == 2) {
                _dk.frame = 0;
                // Spawn barrel on the pounding frame return
                if (random_range(0, 100) < 60) {
                    _barrels.push_back({25.0f, 9.5f, 0.4f + (random_range(0, 100)/500.0f), 0.0f, true, false});
                }
            }
            _dk.lastAnimTime = nowTime;
        }

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

                _mario.vx = (_mario.x < _mario.targetX) ? 0.7f : -0.7f;
                _mario.x += _mario.vx;
                
                // Sloped Walking Logic: Calculate Y based on X and current floor
                // Ground: Flat at y=28 -> Lift to 26 (Feet at 29, Line at 31). Actually Gnd is 31. Feet at 29 is floating?
                // Wait, previous Gnd was 28. Feet at 31. Line at 31. That was correct.
                // User said "Lift Mario UP". If he is currently at 28, feet at 31.
                // Slope T1: Line 27..29. Mario 26..28. Feet 29..31.
                // Feet (29) > Line (27) at X=0. He is IN the line by 2px.
                // So T1 needs -2.0f.
                
                if (_mario.y > 27.5f) { 
                    _mario.y = 28.0f; // Ground remains 28 (Feet 31, Line 31)
                }
                else if (_mario.y > 23.0f) { // T1
                     // Prev: 26.0f + ... -> New: 24.0f + ...
                     // T1 Line: (0, 27) to (Max, 29). 
                     // Mario Y: 24.0f. Feet: 27.0f. Matches Line at X=0.
                     _mario.y = 24.0f + (_mario.x / MATRIX_WIDTH) * 2.0f;
                }
                else if (_mario.y > 17.0f) { // T2
                     // Prev: 20.0f + ... -> New: 18.0f + ...
                     // T2 Line: (Max, 21) to (0, 23).
                     // Mario Y at Max: 20 -> 18. Feet: 21. Matches Line 21.
                     _mario.y = 18.0f + ((MATRIX_WIDTH - _mario.x) / MATRIX_WIDTH) * 2.0f;
                }
                else { // T3
                     // Prev: 14.0f -> New: 12.0f.
                     // T3 Line: (DKEdge+1, 15).
                     // Mario Y: 12.0f. Feet: 15.0f. Matches Line 15.
                     float normalizedX = (_mario.x - (MATRIX_WIDTH/4.0f + 8.0f)) / (MATRIX_WIDTH - (MATRIX_WIDTH/4.0f + 8.0f));
                     if (normalizedX < 0) normalizedX = 0;
                     _mario.y = 12.0f + normalizedX * 2.0f;
                }

                // Jump over barrels
                for (const auto& b : _barrels) {
                    if (abs(b.x - _mario.x) < 4.0f && abs(b.y - (_mario.y + 3)) < 2.0f) {
                        _mario.state = JUMPING; _mario.vy = -1.1f; // Slightly lower jump
                        _mario.jumpFloorY = _mario.y; // Lock the floor we jumped from!
                        debugA("Mario jumped from %.1f\n", _mario.jumpFloorY);
                    }
                }

                // Check ladder proximity - WIDENED WINDOW for v9 + Forced Logic for v10
                bool nearL1 = abs(_mario.x - L1) < 2.5f;
                bool nearL2 = abs(_mario.x - L2) < 2.5f;
                bool nearL3 = abs(_mario.x - L3) < 2.5f;

                // Helper to start climb
                auto startClimb = [&](float ladderX, float targetY) {
                    _mario.state = CLIMBING; 
                    _mario.x = ladderX; // SNAP to ladder center
                    _mario.vy = -0.4f; 
                    _mario.vx = 0;
                    _mario.climbTargetY = targetY; // Explicit target
                    // Pick a random destination X (L1 or L3 usually safe)
                    _mario.targetX = (ladderX == L1) ? L3 : (ladderX == L3 ? L1 : (random_range(0,100)<50?L1:L3));
                    debugA("Mario climbing %.1f to %.1f\n", ladderX, targetY);
                };

                // Targets: Gnd->T1 (25.0), T1->T2 (19.5), T2->T3 (13.0)
                if (nearL2 && _mario.y > 27.0f) { // Ground to T1
                    if (_mario.targetX == L2 || random_range(0, 100) < 80) startClimb(L2, 25.0f); 
                } else if (nearL1 && _mario.y > 24.5f && _mario.y < 28.0f) { // T1 to T2
                    if (_mario.targetX == L1 || random_range(0, 100) < 80) startClimb(L1, 19.5f);
                } else if (nearL3 && _mario.y > 19.5f && _mario.y < 22.0f) { // T2 to T3
                    if (_mario.targetX == L3 || random_range(0, 100) < 80) startClimb(L3, 13.0f);
                }
                
                if (_mario.x > MATRIX_WIDTH - 4 || _mario.x < 4) _mario.vx *= -1.0f;
            }
        } else if (_mario.state == CLIMBING) {
            _mario.y += _mario.vy;
            _mario.frame = 3; // Climbing frame
            
            // Explicit Target Logic - No more fuzzy ranges causing infinite loops!
            if (_mario.vy < 0) { // UP
                if (_mario.y <= _mario.climbTargetY) {
                    _mario.state = WALKING;
                    _mario.y = _mario.climbTargetY;
                    _mario.vy = 0;
                    // Resume walking (direction based on TargetX)
                    _mario.vx = (_mario.x < _mario.targetX) ? 0.6f : -0.6f;
                    debugA("Mario arrived at target %.1f\n", _mario.climbTargetY);
                }
            } else { // Climbing DOWN
                 // Not implemented yet
            }
        } else if (_mario.state == JUMPING) {
            _mario.y += _mario.vy;
            _mario.vy += 0.15f; // Gravity
            // Use locked floorY to prevent deck-hopping
            if (_mario.y >= _mario.jumpFloorY && _mario.vy > 0) {
                _mario.y = _mario.jumpFloorY; _mario.vy = 0; _mario.state = WALKING;
            }
        }

        // Minute change trigger
        auto now_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        struct tm tm_buf;
        int currentMin = localtime_r(&now_t, &tm_buf)->tm_min;
        if (_lastMin == -1) _lastMin = currentMin;
        if (currentMin != _lastMin) {
            _barrels.push_back({25.0f, 9.5f, 0.7f, 0.0f, true, true}); // Blue barrel!
            _lastMin = currentMin;
            _flashAmount = 1.0f; // Trigger flash
        }
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
            b.x += b.vx;
            b.y += b.vy;

            // FSM-based pathing (updated for v7 layout)
            if (b.y < 18 && b.vx > 0) { // T3 Slope (y=15->17)
                b.y = 15.5f + (b.x - 25.0f) * (17.0f - 15.5f) / (MATRIX_WIDTH - 8 - 25.0f);
                if (b.x >= MATRIX_WIDTH - 8) { b.x = MATRIX_WIDTH - 8; b.vx = 0; b.vy = 0.5f; }
            }
            else if (b.vx == 0 && b.vy > 0 && b.y < 22) { // Ladder T3->T2
                if (b.y >= 21.5f) { b.y = 21.5f; b.vx = -0.5f; b.vy = 0; }
            }
            else if (b.vx < 0 && b.y < 24) { // T2 Slope (y=21->23)
                b.y = 21.5f + (MATRIX_WIDTH - 8 - b.x) * (23.0f - 21.5f) / (MATRIX_WIDTH - 8 - 8.0f);
                if (b.x <= 8) { b.x = 8; b.vx = 0; b.vy = 0.5f; }
            }
            else if (b.vx == 0 && b.vy > 0 && b.y < 28) { // Ladder T2->T1
                // 27.5f was causing 1px drop. T1 slope starts at 27.5f at x=8. Ground ladder is at center?
                // T1 Line: (0, 27) -> (W, 29). At center (16/32), it's 28.
                // Snap to correct slope height at ladder exit
                if (b.y >= 27.5f) { b.y = 27.5f; b.vx = 0.5f; b.vy = 0; }
            }
            else if (b.vx > 0 && b.y < 30) { // T1 Slope (y=27->29)
                b.y = 27.5f + (b.x - 8.0f) * (29.5f - 27.5f) / (MATRIX_WIDTH - 16 - 8.0f);
                if (b.x >= MATRIX_WIDTH - 16) { b.x = MATRIX_WIDTH - 16; b.vx = 0; b.vy = 0.5f; }
            }
            else if (b.vx == 0 && b.vy > 0 && b.y >= 27.5f) { // Ladder T1->Ground
                if (b.y >= 31.0f) { b.y = 31.0f; b.vx = (random_range(0, 100) > 50 ? 0.5f : -0.5f); b.vy = 0; }
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
