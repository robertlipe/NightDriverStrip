#pragma once

#include "effects.h"
#include <vector>
#include <string>
#include <algorithm>

// Entity for bullets and invaders
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
    float swarmSpeed = 0.2f;
    int score = 0;

public:
    PatternCGInvade() : EffectWithId<PatternCGInvade>("CG Invaders") {
        ResetGame();
    }

    PatternCGInvade(const JsonObjectConst& jsonObject) : EffectWithId<PatternCGInvade>(jsonObject) {
        ResetGame();
    }

    void Start() override {
        ResetGame();
    }

    void ResetGame() {
        invaders.clear();
        bullets.clear();

        // Player at bottom center
        player = {32.0f, 30.0f, 0.0f, 0.0f, true, 0x00FF00};

        // Initializing the "Time" armada or classic grid
        // Logic derived from datagod/LEDarcade
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 12; col++) {
                invaders.push_back({(float)col * 5 + 4, (float)row * 6 + 2, 0, 0, true, 0xFFFFFF});
            }
        }
    }

    void Draw() override {
        // 1. Persistence of Vision / Trail effect
        fadeAllChannelsToBlackBy(55); // Scale 0-255

        // 2. Update and Draw Player
        // If WebHID tilt data arrives, player.vx is set there
        player.x += player.vx;
        player.x = std::clamp(player.x, 0.0f, (float)MATRIX_WIDTH - 1.0f);
        
        g()->setPixel((int)player.x, (int)player.y, CRGB(player.color));
        g()->setPixel((int)player.x-1, (int)player.y, CRGB(player.color));
        g()->setPixel((int)player.x+1, (int)player.y, CRGB(player.color));

        // 3. Update/Draw Bullets (Sub-pixel movement)
        for (auto& b : bullets) {
            if (!b.active) continue;
            b.y -= 0.8f; // Bullet speed
            if (b.y < 0) b.active = false;
            g()->setPixel((int)b.x, (int)b.y, CRGB(b.color));
        }

        // 4. Update/Draw Invaders
        bool shiftDown = false;
        for (auto& inv : invaders) {
            if (!inv.active) continue;

            inv.x += swarmDirection * swarmSpeed;
            if (inv.x > (MATRIX_WIDTH - 4) || inv.x < 2) shiftDown = true;

            // Draw iconic 3x3 or 5x5 shapes
            DrawInvader((int)inv.x, (int)inv.y, inv.color);
        }

        if (shiftDown) {
            swarmDirection *= -1;
            for (auto& inv : invaders) inv.y += 1.0f;
        }

        // 5. Collision Detection (The "Math" part)
        HandleCollisions();
    }

    void DrawInvader(int x, int y, uint32_t col) {
        // Simple 3x3 pixel "Crab"
        CRGB color = CRGB(col);
        g()->setPixel(x, y, color);
        g()->setPixel(x-1, y+1, color);
        g()->setPixel(x+1, y+1, color);
        g()->setPixel(x-1, y-1, color);
        g()->setPixel(x+1, y-1, color);
    }

    void HandleCollisions() {
        for (auto& b : bullets) {
            if (!b.active) continue;
            for (auto& inv : invaders) {
                if (!inv.active) continue;
                // Simple AABB collision
                if (abs(b.x - inv.x) < 2 && abs(b.y - inv.y) < 2) {
                    inv.active = false;
                    b.active = false;
                    score += 10;
                }
            }
        }
    }
};
