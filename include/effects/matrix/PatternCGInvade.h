#pragma once

#include "effects.h"
#include <vector>
#include <chrono>
#include <algorithm>
#include <cmath>

struct CGEntity {
    float x, y;
    float vx, vy;
    uint32_t color;
    bool active;
    int life = 255;
};

class PatternCGInvade : public EffectWithId<PatternCGInvade> {
private:
    // Named Constants to kill the "Magic"
    static constexpr float kBulletSpeed = 0.9f;
    static constexpr float kSwarmBaseSpeed = 0.16f;
    static constexpr float kPlayerSpeed = 0.8f;
    static constexpr int kDebrisFadeRate = 12;
    static constexpr float kCollisionRadius = 0.6f;

    std::vector<CGEntity> swarm;
    std::vector<CGEntity> bullets;
    std::vector<CGEntity> debris;
    std::vector<CGEntity> bunkers;
    CGEntity ufo;
    CGEntity player;

    float swarmDir = 1.0f;
    float swarmSpeed = kSwarmBaseSpeed;
    uint32_t lastShot = 0;
    uint32_t nextShotDelay = 400;
    uint32_t lastTargetChange = 0;
    float currentAITargetX = 32.0f;
    int lastMin = -1;

    const uint32_t font3x5[11][5] = {
        {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
        {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
        {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
        {0b111, 0b001, 0b111, 0b001, 0b111}, // 3
        {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
        {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
        {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
        {0b111, 0b001, 0b010, 0b100, 0b100}, // 7
        {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
        {0b111, 0b101, 0b111, 0b001, 0b111}, // 9
        {0b000, 0b010, 0b000, 0b010, 0b000}  // :
    };

public:
    PatternCGInvade() : EffectWithId<PatternCGInvade>("CG Invaders") { ResetGame(); }
    PatternCGInvade(const JsonObjectConst& json) : EffectWithId<PatternCGInvade>(json) { ResetGame(); }

    void Start() override { ResetGame(); }

    void ResetGame() {
        bullets.clear(); swarm.clear(); debris.clear(); bunkers.clear();
        swarmSpeed = kSwarmBaseSpeed; swarmDir = 1.0f;
        ufo.active = false;
        player = {(float)MATRIX_WIDTH/2.0f, (float)MATRIX_HEIGHT - 2.0f, 0, 0, (uint32_t)CRGB::Green, true};
        SpawnTimeArmada();
        SpawnBunkers();
    }

    void SpawnBunkers() {
        for (int b = 0; b < 4; b++) {
            int bx = 6 + (b * 16);
            for (int x = 0; x < 5; x++) {
                for (int y = 0; y < 3; y++) {
                    if (!((y == 2 && (x == 1 || x == 2 || x == 3)) || (y == 0 && (x == 0 || x == 4))))
                        bunkers.push_back({(float)bx + x, (float)MATRIX_HEIGHT - 7.0f + y, 0, 0, (uint32_t)CRGB::Green, true});
                }
            }
        }
    }

    void SpawnTimeArmada() {
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        struct tm* t = localtime(&now);
        lastMin = t->tm_min;
        int digits[5] = {t->tm_hour/10, t->tm_hour%10, 10, t->tm_min/10, t->tm_min%10};
        int startX = (MATRIX_WIDTH - (5 * 4)) / 2;

        for (int d = 0; d < 5; d++) {
            for (int row = 0; row < 5; row++) {
                uint32_t rowBits = font3x5[digits[d]][row];
                for (int col = 0; col < 3; col++) {
                    if ((rowBits >> (2 - col)) & 1) {
                        float px = startX + (d * 4) + col;
                        float py = 2 + row;
                        uint32_t colr = (d < 2) ? (uint32_t)CRGB::Cyan : (d == 2) ? (uint32_t)CRGB::White : (uint32_t)CRGB::Green;
                        swarm.push_back({px, py, 0, 0, colr, true});
                    }
                }
            }
        }
    }

    void Draw() override {
        fadeAllChannelsToBlackBy(80);

        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        if (localtime(&now)->tm_min != lastMin || IsBoardClear()) {
            ResetGame(); return;
        }

        UpdateAI();
        UpdateUFO();

        bool hitWall = false;
        for (auto& p : swarm) {
            if (!p.active) continue;
            p.x += swarmDir * swarmSpeed;
            if (p.x >= MATRIX_WIDTH - 2 || p.x <= 1) hitWall = true;
            if (p.y < player.y) safeSet(p.x, p.y, p.color);
            else { ResetGame(); return; }
        }
        if (hitWall) {
            swarmDir *= -1;
            for (auto& p : swarm) p.y += 1.0f;
            if (swarmSpeed < 0.5f) swarmSpeed += 0.005f;
        }

        for (auto& b : bullets) {
            if (!b.active) continue;
            b.y -= kBulletSpeed;
            if (b.y < 0) { b.active = false; continue; }
            safeSet(b.x, b.y, b.color);

            for (auto& bk : bunkers) {
                if (bk.active && abs(b.x - bk.x) < 0.7f && abs(b.y - bk.y) < 0.7f) {
                    bk.active = b.active = false;
                    SpawnExplosion(bk.x, bk.y, bk.color, 2);
                    break;
                }
            }
            if (!b.active) continue;

            for (auto& p : swarm) {
                if (p.active && abs(b.x - p.x) < kCollisionRadius && abs(b.y - p.y) < kCollisionRadius) {
                    p.active = b.active = false;
                    SpawnExplosion(p.x, p.y, p.color, 4);
                    break;
                }
            }
        }

        for (const auto& bk : bunkers) if (bk.active) safeSet(bk.x, bk.y, bk.color);
        for (auto& d : debris) {
            if (!d.active) continue;
            d.x += d.vx; d.y += d.vy; d.vy += 0.02f;
            d.life -= kDebrisFadeRate;
            if (d.life <= 0) d.active = false;
            else safeSet(d.x, d.y, CRGB(d.color).nscale8(d.life));
        }

        int px = (int)round(player.x);
        int py = (int)round(player.y);
        safeSet(px, py, CRGB(player.color));
        safeSet(px - 1, py, CRGB(player.color));
        safeSet(px + 1, py, CRGB(player.color));
        safeSet(px, py - 1, CRGB(player.color));

        Cleanup();
    }

    void UpdateUFO() {
        if (!ufo.active && random(3500) < 5) {
            ufo = { (float)MATRIX_WIDTH, 1.0f, -0.45f, 0, (uint32_t)CRGB::Red, true };
        }
        if (ufo.active) {
            ufo.x += ufo.vx;
            if (ufo.x < -5) ufo.active = false;
            else {
                safeSet(ufo.x, ufo.y, ufo.color);
                safeSet(ufo.x-1, ufo.y+1, ufo.color);
                safeSet(ufo.x, ufo.y+1, ufo.color);
                safeSet(ufo.x+1, ufo.y+1, ufo.color);
            }
        }
    }

    void UpdateAI() {
        if (millis() - lastTargetChange > 1500 || !TargetStillExists(currentAITargetX)) {
            float highestY = -1.0f;
            std::vector<float> candidates;
            for (const auto& p : swarm) {
                if (p.active) {
                    if (p.y > highestY) {
                        highestY = p.y;
                        candidates.clear();
                        candidates.push_back(p.x);
                    } else if (p.y == highestY) {
                        candidates.push_back(p.x);
                    }
                }
            }
            if (!candidates.empty()) {
                currentAITargetX = candidates[random(candidates.size())];
                lastTargetChange = millis();
            }
        }

        float sweep = sin(millis() / 400.0f) * 2.5f;
        float dest = currentAITargetX + sweep;
        if (player.x < dest) player.x += kPlayerSpeed;
        else if (player.x > dest) player.x -= kPlayerSpeed;
        player.x = std::clamp(player.x, 1.0f, (float)MATRIX_WIDTH - 2.0f);

        if (millis() - lastShot > nextShotDelay) {
            bullets.push_back({player.x, player.y - 1.0f, 0, 0, (uint32_t)CRGB::White, true});
            lastShot = millis();
            nextShotDelay = 250 + random(500);
        }
    }

    bool TargetStillExists(float x) {
        for(const auto& p : swarm) if(p.active && abs(p.x - x) < 2.0f) return true;
        return false;
    }

    void safeSet(float x, float y, CRGB c) {
        int ix = (int)round(x);
        int iy = (int)round(y);
        if (ix >= 0 && ix < MATRIX_WIDTH && iy >= 0 && iy < MATRIX_HEIGHT) {
            g()->setPixel(ix, iy, c);
        }
    }

    void SpawnExplosion(float x, float y, uint32_t col, int count) {
        for (int i = 0; i < count; i++) {
            debris.push_back({x, y, ((float)random(100)-50)/60.0f, ((float)random(100)-90)/90.0f, col, true, 255});
        }
    }

    bool IsBoardClear() {
        for(const auto& p : swarm) if(p.active) return false;
        return true;
    }

    void Cleanup() {
        if (bullets.size() > 20) bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const CGEntity& b){ return !b.active; }), bullets.end());
        if (debris.size() > 70) debris.erase(std::remove_if(debris.begin(), debris.end(), [](const CGEntity& d){ return !d.active; }), debris.end());
    }
};
