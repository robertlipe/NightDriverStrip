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

    struct Entity {
        float x, y;
        uint8_t frame;
        uint32_t lastAnimTime;
    };

    std::vector<Barrel> _barrels;
    Entity _dk;
    Entity _mario;
    int _lastMin = -1;

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

    static constexpr uint8_t dkSprite[3][10] = {
        {0x0F, 0x1F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x1F, 0x0F}, // Stand
        {0x0F, 0x1F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x1F, 0x0F}, // Arms out
        {0x0F, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x3F, 0x0F}  // Pounding
    };

    static constexpr uint8_t marioSprite[4][8] = {
        {0x1C, 0x3E, 0x36, 0x3E, 0x08, 0x14, 0x14, 0x00}, // Stand
        {0x1C, 0x3E, 0x36, 0x3E, 0x18, 0x0C, 0x06, 0x00}, // Run A
        {0x1C, 0x3E, 0x36, 0x3E, 0x18, 0x30, 0x60, 0x00}, // Run B
        {0x1C, 0x3E, 0x36, 0x3E, 0x18, 0x14, 0x22, 0x00}  // Jump
    };

    static constexpr uint32_t kRed = 0xAA0000;

public:
    PatternCGKong() : EffectWithId<PatternCGKong>("CG Kong") { ResetGame(); }
    PatternCGKong(const JsonObjectConst& json) : EffectWithId<PatternCGKong>(json) { ResetGame(); }

    void Start() override { ResetGame(); }

    void ResetGame() {
        _barrels.clear();
        _dk = {12.0f, 6.0f, 0, 0};
        _mario = {32.0f, (float)MATRIX_HEIGHT - 3.0f, 0, 0};
        _lastMin = -1;
    }

    void Draw() override {
        g()->DimAll(160);
        DrawGirders();
        DrawGhostClock();
        UpdateEntities();
        DrawEntities();
        UpdateBarrels();
        DrawBarrels();
    }

private:
    void DrawGirders() {
        uint32_t girderCol = kRed;
        g()->drawLine(0, 7, 24, 7, CRGB(girderCol)); // DK platform
        g()->drawLine(25, 9, MATRIX_WIDTH - 1, 11, CRGB(girderCol)); // T3
        g()->drawLine(MATRIX_WIDTH - 1, 19, 0, 21, CRGB(girderCol)); // T2
        g()->drawLine(0, 27, MATRIX_WIDTH - 1, 29, CRGB(girderCol)); // T1
        g()->drawLine(0, 31, MATRIX_WIDTH - 1, 31, CRGB(girderCol)); // Ground

        DrawLadder(MATRIX_WIDTH - 8, 11, 8); // T3 to T2
        DrawLadder(8, 21, 8);               // T2 to T1
        DrawLadder(MATRIX_WIDTH - 16, 29, 2); // T1 to Ground
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
            if (highlight > 0) {
                uint8_t val = (uint8_t)(180.0f * highlight);
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
        // DK Pounding
        if (nowTime - _dk.lastAnimTime > 600) {
            _dk.frame = (_dk.frame + 1) % 3;
            _dk.lastAnimTime = nowTime;
            // Spawn barrel on the pounding frame
            if (_dk.frame == 2 && random(100) < 40) {
                _barrels.push_back({25.0f, 9.5f, 0.4f + (random(100)/500.0f), 0.0f, true, false});
            }
        }

        // Mario Movement (Simple run back and forth)
        static float marioDir = 1.0f;
        if (nowTime - _mario.lastAnimTime > 150) {
            _mario.frame = (_mario.frame + 1) % 3; // Cycle 0,1,2 (Stand, RunA, RunB)
            _mario.lastAnimTime = nowTime;
            _mario.x += marioDir * 0.8f;
            if (_mario.x > MATRIX_WIDTH - 8 || _mario.x < 4) {
                marioDir *= -1.0f;
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
        }
    }

    void DrawEntities() {
        // DK
        for (int r = 0; r < 10; r++) {
            uint8_t bits = dkSprite[_dk.frame][r];
            for (int c = 0; c < 8; c++) {
                if ((bits >> (7 - c)) & 1) g()->setPixel(_dk.x + c - 4, _dk.y + r - 4, CRGB(CRGB::Brown));
            }
        }
        // Mario
        for (int r = 0; r < 8; r++) {
            uint8_t bits = marioSprite[_mario.frame][r];
            for (int c = 0; c < 8; c++) {
                if ((bits >> (7 - c)) & 1) g()->setPixel(_mario.x + c - 4, _mario.y + r - 4, CRGB(CRGB::Red));
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
                if (b.y >= 31.0f) { b.y = 31.0f; b.vx = (random(2) ? 0.5f : -0.5f); b.vy = 0; }
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
