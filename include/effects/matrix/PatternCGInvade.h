#pragma once

#include "effects.h"
#include <vector>
#include <chrono>
#include <algorithm>

struct GameEntity {
    float x, y;
    float vx, vy;
    bool active;
    uint32_t color;
};

class PatternCGInvade : public EffectWithId<PatternCGInvade> {
private:
    std::vector<GameEntity> invaders;
    std::vector<GameEntity> bullets;
    GameEntity player;

    float swarmDirection = 1.0f;
    float swarmSpeed = 0.15f;
    uint32_t lastFireTime = 0;
    const uint32_t fireDelay = 400; // Fast firing for voxel cleanup
    int lastMin = -1;

    // Minimalist 5x7 Font Bitmaps (0-9 and :)
    const uint8_t font5x7[11][7] = {
        {0x3E, 0x41, 0x41, 0x41, 0x3E}, // 0
        {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
        {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
        {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
        {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
        {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
        {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
        {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
        {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
        {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
        {0x00, 0x36, 0x36, 0x00, 0x00}  // :
    };

public:
    PatternCGInvade() : EffectWithId<PatternCGInvade>("CG Invaders") { ResetGame(); }
    PatternCGInvade(const JsonObjectConst& json) : EffectWithId<PatternCGInvade>(json) { ResetGame(); }

    void ResetGame() {
        invaders.clear();
        bullets.clear();
        player = {(float)MATRIX_WIDTH/2, (float)MATRIX_HEIGHT-2, 0, 0, true, 0x00FF00};
        SpawnTimeArmada();
    }

    void SpawnTimeArmada() {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        struct tm* ltm = localtime(&now);
        lastMin = ltm->tm_min;

        int digits[5] = {ltm->tm_hour/10, ltm->tm_hour%10, 10, ltm->tm_min/10, ltm->tm_min%10};
        int startX = (MATRIX_WIDTH - (5 * 6)) / 2;

        for (int d = 0; d < 5; d++) {
            for (int x = 0; x < 5; x++) {
                uint8_t line = font5x7[digits[d]][x];
                for (int y = 0; y < 7; y++) {
                    if (line & (1 << y)) {
                        uint32_t col = (y < 3) ? 0xFF0000 : (y < 5) ? 0x00FF00 : 0x00FFFF;
                        invaders.push_back({(float)(startX + (d * 6) + x), (float)(y + 4), 0, 0, true, col});
                    }
                }
            }
        }
    }

    void Draw() override {
        // High-speed persistence trails
        fadeAllChannelsToBlackBy(70);

        // 1. Check for New Minute or Level Clear
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if (localtime(&now)->tm_min != lastMin || AllInvadersDead()) {
            ResetGame();
            return;
        }

        // 2. Self-Playing AI: Find Centroid of remaining pixels
        float avgX = 0, count = 0, lowestY = 0;
        for (auto& inv : invaders) {
            if (inv.active) {
                avgX += inv.x;
                count++;
                if (inv.y > lowestY) lowestY = inv.y;
            }
        }
        if (count > 0) {
            float targetX = avgX / count;
            if (player.x < targetX) player.x += 0.4f;
            else if (player.x > targetX) player.x -= 0.4f;
        }

        // 3. Auto-Fire
        if (millis() - lastFireTime > fireDelay) {
            bullets.push_back({player.x, player.y - 1, 0, 0, true, 0xFFFFFF});
            lastFireTime = millis();
        }

        // 4. Update Swarm Movement
        bool shiftDown = false;
        for (auto& inv : invaders) {
            if (!inv.active) continue;
            inv.x += swarmDirection * swarmSpeed;
            if (inv.x >= MATRIX_WIDTH - 1 || inv.x <= 0) shiftDown = true;
            
            // Safety-checked Draw
            if (inv.y < MATRIX_HEIGHT) g()->setPixel((int)inv.x, (int)inv.y, CRGB(inv.color));
        }

        if (shiftDown) {
            swarmDirection *= -1;
            for (auto& inv : invaders) {
                inv.y += 1.0f;
                if (inv.y >= player.y) { ResetGame(); return; } // Avoid Mayhem
            }
        }

        // 5. Update Bullets & Collisions
        for (auto& b : bullets) {
            if (!b.active) continue;
            b.y -= 0.8f;
            if (b.y < 0) b.active = false;
            else g()->setPixel((int)b.x, (int)b.y, CRGB(b.color));

            for (auto& inv : invaders) {
                if (inv.active && abs(b.x - inv.x) < 1.0f && abs(b.y - inv.y) < 1.0f) {
                    inv.active = false;
                    b.active = false;
                    break;
                }
            }
        }

        // 6. Draw Player Cannon
        g()->setPixel((int)player.x, (int)player.y, CRGB(player.color));
        g()->setPixel((int)player.x - 1, (int)player.y, CRGB(player.color));
        g()->setPixel((int)player.x + 1, (int)player.y, CRGB(player.color));
        
        CleanupVectors();
    }

    bool AllInvadersDead() {
        return std::all_of(invaders.begin(), invaders.end(), [](const GameEntity& i) { return !i.active; });
    }

    void CleanupVectors() {
        if (bullets.size() > 30) {
            bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const GameEntity& b){return !b.active;}), bullets.end());
        }
    }
};
