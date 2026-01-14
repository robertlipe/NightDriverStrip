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

    // DK Fur (Dark Brown) 12x12 - More shoulders and head distinction
    static constexpr uint16_t dkFur[3][12] = {
        {0x000, 0x1E0, 0x1E0, 0x7F8, 0xFFC, 0xFFC, 0xFFC, 0xFFC, 0x7F8, 0x7F8, 0x000, 0x000}, // Shoulders + head
        {0x000, 0x1E0, 0x1E0, 0xFFC, 0xFFC, 0xFFC, 0xFFC, 0xFFC, 0xFFC, 0xFFC, 0x000, 0x000}, // Arms wide
        {0xFFC, 0xFFC, 0xFFC, 0xFFC, 0xFFC, 0xFFC, 0xFFC, 0xFFC, 0xFFC, 0xFFC, 0x000, 0x000}  // Arms up (pounding)
    };
    // DK Skin (Tan) - Eyes/Mouth/Chest
    static constexpr uint16_t dkTan[3][12] = {
        {0x000, 0x000, 0x120, 0x000, 0x0C0, 0x000, 0x1E0, 0x1E0, 0x000, 0x000, 0x000, 0x000}, // Eyes (1), Mouth (1E0)
        {0x000, 0x000, 0x120, 0x000, 0x0C0, 0x000, 0x1E0, 0x1E0, 0x000, 0x000, 0x000, 0x000},
        {0x000, 0x000, 0x120, 0x000, 0x1E0, 0x000, 0x3F0, 0x3F0, 0x000, 0x000, 0x000, 0x000}
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
        _dk = {12.0f, 6.0f, 0, 0};
        _mario = {32.0f, (float)MATRIX_HEIGHT - 3.0f, 0, 0};
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
        g()->drawLine(0, 7, 24, 7, CRGB(girderCol)); // DK platform
        g()->drawLine(0, 8, 24, 8, CRGB(girderCol)); // Thicken
        
        // Use a loop to draw 2-pixel thick slanted girders for solidity
        auto drawThickLine = [&](int x1, int y1, int x2, int y2) {
            g()->drawLine(x1, y1, x2, y2, CRGB(girderCol));
            g()->drawLine(x1, y1 + 1, x2, y2 + 1, CRGB(girderCol));
        };

        drawThickLine(25, 9, MATRIX_WIDTH - 1, 11); // T3
        drawThickLine(MATRIX_WIDTH - 1, 19, 0, 21); // T2
        drawThickLine(0, 27, MATRIX_WIDTH - 1, 29); // T1
        g()->drawLine(0, 31, MATRIX_WIDTH - 1, 31, CRGB(girderCol)); // Ground

        DrawLadder(MATRIX_WIDTH - 8, 11, 8); // T3 to T2
        DrawLadder(8, 21, 8);               // T2 to T1
        DrawLadder(MATRIX_WIDTH - 24, 29, 2); // T1 to Ground
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
                if (abs(_mario.x - _mario.targetX) < 1.0f) {
                    _mario.targetX = (_mario.targetX == 8.0f) ? (MATRIX_WIDTH - 24.0f) : 8.0f;
                }
                _mario.vx = (_mario.x < _mario.targetX) ? 0.7f : -0.7f;
                _mario.x += _mario.vx;
                
                // Check ladder proximity (T1 ladder at MW-24, T2 ladder at 8)
                if (abs(_mario.x - (MATRIX_WIDTH - 24)) < 2.0f && _mario.y > 30.0f) {
                    if (random_range(0, 100) < 80) { // 80% chance to climb
                        _mario.state = CLIMBING; _mario.x = MATRIX_WIDTH - 24; _mario.vy = -0.4f; _mario.vx = 0;
                        _mario.targetX = 8.0f; // Next target on T1
                    }
                } else if (abs(_mario.x - 8) < 2.0f && _mario.y > 27.5f && _mario.y < 28.5f) {
                    if (random_range(0, 100) < 80) { // 80% chance to climb
                        _mario.state = CLIMBING; _mario.x = 8; _mario.vy = -0.4f; _mario.vx = 0;
                        _mario.targetX = MATRIX_WIDTH - 8.0f; // Next target on T2
                    }
                }
                
                if (_mario.x > MATRIX_WIDTH - 8 || _mario.x < 4) _mario.vx *= -1.0f;
            }
        } else if (_mario.state == CLIMBING) {
            _mario.y += _mario.vy;
            _mario.frame = 3; // Climbing frame
            if (_mario.y <= 27.5f && _mario.y > 26.0f) { // Reached T1
                _mario.state = WALKING; _mario.y = 27.5f; _mario.vy = 0; _mario.vx = 0.6f;
            } else if (_mario.y <= 19.5f) { // Reached T2
                _mario.state = WALKING; _mario.y = 19.5f; _mario.vy = 0; _mario.vx = 0.6f;
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
                if ((rBits >> (7 - c)) & 1) col = CRGB::Red;
                else if ((bBits >> (7 - c)) & 1) col = CRGB::Blue;
                else if ((sBits >> (7 - c)) & 1) col = skinCol;
                
                if (col != CRGB::Black) g()->drawPixelXYF_Wu(_mario.x + c - 4, _mario.y + r - 4, col);
            }
        }
    }

    void UpdateBarrels() {
        for (auto& b : _barrels) {
            if (!b.active) continue;
            b.x += b.vx;
            b.y += b.vy;

            // FSM-based pathing
            if (b.y < 12 && b.vx > 0) { // T3 Slope
                b.y = 9.5f + (b.x - 25.0f) * (11.0f - 9.5f) / (MATRIX_WIDTH - 8 - 25.0f);
                if (b.x >= MATRIX_WIDTH - 8) { b.x = MATRIX_WIDTH - 8; b.vx = 0; b.vy = 0.5f; }
            }
            else if (b.vx == 0 && b.vy > 0 && b.y < 20) { // Ladder T3->T2
                if (b.y >= 19.5f) { b.y = 19.5f; b.vx = -0.5f; b.vy = 0; }
            }
            else if (b.vx < 0 && b.y < 23) { // T2 Slope
                b.y = 19.5f + (MATRIX_WIDTH - 8 - b.x) * (21.0f - 19.5f) / (MATRIX_WIDTH - 8 - 8.0f);
                if (b.x <= 8) { b.x = 8; b.vx = 0; b.vy = 0.5f; }
            }
            else if (b.vx == 0 && b.vy > 0 && b.y < 28) { // Ladder T2->T1
                if (b.y >= 27.5f) { b.y = 27.5f; b.vx = 0.5f; b.vy = 0; }
            }
            else if (b.vx > 0 && b.y < 30) { // T1 Slope
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
            if (b.active) {
                g()->drawPixelXYF_Wu(b.x, b.y, b.isBlue ? CRGB(CRGB::Blue) : CRGB(CRGB::Orange));
            }
        }
    }
};

#endif // PATTERN_CG_KONG_H
