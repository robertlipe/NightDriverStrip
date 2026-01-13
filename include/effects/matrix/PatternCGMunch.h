#pragma once

#include "effects.h"
#include <bitset>

/**
 * PatternCGMunch: A Pac-Man inspired clock for 64x32 LED matrices.
 * Features grid-locked movement, scent-trail ghost AI, and 3x5 font.
 */

enum MunchDir { UP, RIGHT, DOWN, LEFT, NONE };

struct MunchEntity {
    int32_t x, y;
    MunchDir dir;
};

class PatternCGMunch : public EffectWithId<PatternCGMunch> {
private:
    MunchEntity munch;
    MunchEntity blinky;

    // Trail of recent logical positions for the ghost to follow
    int16_t trailX[12], trailY[12];
    int trailIdx = 0;
    int lastMin = -1;

    static constexpr int MAZE_W = 28;
    static constexpr int MAZE_H = 31;
    static constexpr int SCALE  = 256;

    // Entity Speeds (scaled)
    static constexpr int32_t kMunchSpeed  = 64;
    static constexpr int32_t kBlinkySpeed = 48;

    // Maze Geometry Constants
    static constexpr int kHouseTop = 12;
    static constexpr int kHouseBot = 16;
    static constexpr int kHouseL   = 10;
    static constexpr int kHouseR   = 17;

    std::bitset<MAZE_W * MAZE_H> pellets;

    const uint32_t mazeMask[31] = {
        0xFFFFFFF, 0x8000001, 0x8EEEEE1, 0x8EEEEE1, 0x8000001,
        0x8EEEEE1, 0x8E888E1, 0x8E888E1, 0x8000001, 0xFFF8FFF,
        0x0004000, 0x0004000, 0x8000001, 0x8E000E1, 0x8E000E1,
        0x8E000E1, 0x8000001, 0x8EEEE01, 0x800E001, 0x8EEBE01,
        0x8000001, 0x8EEEEE1, 0x800E001, 0x8EEBE01, 0x8000001,
        0x800E001, 0x8EEBE01, 0x8000001, 0x8EEEEE1, 0x8000001,
        0xFFFFFFF
    };

    const uint8_t font3x5[10][5] = {
        {0b111, 0b101, 0b101, 0b101, 0b111}, {0b010, 0b110, 0b010, 0b010, 0b111},
        {0b111, 0b001, 0b111, 0b100, 0b111}, {0b111, 0b001, 0b111, 0b001, 0b111},
        {0b101, 0b101, 0b111, 0b001, 0b001}, {0b111, 0b100, 0b111, 0b001, 0b111},
        {0b111, 0b100, 0b111, 0b101, 0b111}, {0b111, 0b001, 0b010, 0b100, 0b100},
        {0b111, 0b101, 0b111, 0b101, 0b111}, {0b111, 0b101, 0b111, 0b001, 0b111}
    };

    bool isWall(int x, int y) {
        if (y < 0 || y >= MAZE_H) return true;
        // The wrap-around tunnel is only allowed at the equator
        bool isTunnelRow = (y >= 9 && y <= 11);
        int lx = (x + MAZE_W) % MAZE_W;
        if (!isTunnelRow && (x < 0 || x >= MAZE_W)) return true;
        return (mazeMask[y] >> (27 - lx)) & 1;
    }

    void ResetPellets() {
        pellets.reset();
        for (int y = 0; y < MAZE_H; y++) {
            for (int x = 0; x < MAZE_W; x++) {
                if (!isWall(x, y)) pellets.set(y * MAZE_W + x);
            }
        }
    }

    void DrawDigit(int x, int y, int digit, uint16_t color) {
        if (digit < 0 || digit > 9) return;
        for (int r = 0; r < 5; r++) {
            uint8_t bits = font3x5[digit][r];
            for (int c = 0; c < 3; c++) {
                if ((bits >> (2 - c)) & 1) g()->setPixel(x + c, y + r, color);
            }
        }
    }

public:
    PatternCGMunch() : EffectWithId<PatternCGMunch>("Pac-Clock") { ResetGame(); }
    PatternCGMunch(const JsonObjectConst& json) : EffectWithId<PatternCGMunch>(json) { ResetGame(); }

    void ResetGame() {
        munch.x = 14 * SCALE; munch.y = 23 * SCALE; munch.dir = LEFT;
        blinky.x = 14 * SCALE; blinky.y = 14 * SCALE; blinky.dir = UP;
        for (int i = 0; i < 12; i++) { trailX[i] = 14; trailY[i] = 23; }
        trailIdx = 0;
        ResetPellets();
    }

    void UpdateMunch() {
        int tx = munch.x / SCALE, ty = munch.y / SCALE;

        if ((munch.x % SCALE == 0) && (munch.y % SCALE == 0)) {
            pellets.reset(ty * MAZE_W + tx);
            trailX[trailIdx] = (int16_t)tx; trailY[trailIdx] = (int16_t)ty;
            trailIdx = (trailIdx + 1) % 12;

            MunchDir options[4]; int count = 0;
            if (!isWall(tx, ty - 1)) options[count++] = UP;
            if (!isWall(tx + 1, ty)) options[count++] = RIGHT;
            if (!isWall(tx, ty + 1)) options[count++] = DOWN;
            if (!isWall(tx - 1, ty)) options[count++] = LEFT;

            bool straight = false;
            for (int i = 0; i < count; i++) if (options[i] == munch.dir) straight = true;

            if (straight && count <= 2 && random(100) < 85) {
                // Hallway Persistence
            } else if (count > 0) {
                MunchDir opposites[] = { DOWN, LEFT, UP, RIGHT, NONE };
                MunchDir choices[4]; int cCount = 0;
                for (int i = 0; i < count; i++)
                    if (options[i] != opposites[munch.dir]) choices[cCount++] = options[i];

                if (cCount > 0) munch.dir = choices[random(cCount)];
                else munch.dir = options[0];
            }
        }
        int dx = (munch.dir == RIGHT) - (munch.dir == LEFT), dy = (munch.dir == DOWN) - (munch.dir == UP);
        if (!isWall(tx + dx, ty + dy)) { munch.x += dx * kMunchSpeed; munch.y += dy * kMunchSpeed; }
        else { munch.dir = (MunchDir)random(4); }

        if (munch.x < 0) munch.x = (MAZE_W - 1) * SCALE;
        if (munch.x >= MAZE_W * SCALE) munch.x = 0;
    }

    void UpdateBlinky() {
        int tx = blinky.x / SCALE, ty = blinky.y / SCALE;
        // Collision: Return to house if caught
        if (abs(blinky.x - munch.x) < 128 && abs(blinky.y - munch.y) < 128) {
            blinky.x = 14 * SCALE; blinky.y = 14 * SCALE; return;
        }

        if ((blinky.x % SCALE == 0) && (blinky.y % SCALE == 0)) {
            int tIdx = (trailIdx + 6) % 12;
            int targetTX = trailX[tIdx], targetTY = trailY[tIdx];
            if (abs(tx - targetTX) > 10) { targetTX = 14; targetTY = 14; }
            MunchDir options[4]; int count = 0;
            if (!isWall(tx, ty - 1)) options[count++] = UP;
            if (!isWall(tx + 1, ty)) options[count++] = RIGHT;
            if (!isWall(tx, ty + 1)) options[count++] = DOWN;
            if (!isWall(tx - 1, ty)) options[count++] = LEFT;
            if (count > 0) {
                MunchDir bestDir = options[0]; int32_t minDist = 100000;
                MunchDir opposites[] = { DOWN, LEFT, UP, RIGHT, NONE };
                for (int i = 0; i < count; i++) {
                    if (count > 1 && options[i] == opposites[blinky.dir]) continue;
                    int nx = tx + (options[i] == RIGHT) - (options[i] == LEFT);
                    int ny = ty + (options[i] == DOWN) - (options[i] == UP);
                    int32_t d = abs(nx - targetTX) + abs(ny - targetTY);
                    if (d < minDist) { minDist = d; bestDir = options[i]; }
                }
                blinky.dir = bestDir;
            }
        }
        int dx = (blinky.dir == RIGHT) - (blinky.dir == LEFT), dy = (blinky.dir == DOWN) - (blinky.dir == UP);
        if (!isWall(tx + dx, ty + dy)) { blinky.x += dx * kBlinkySpeed; blinky.y += dy * kBlinkySpeed; }
        else { blinky.dir = (MunchDir)random(4); }
        if (blinky.x < 0) blinky.x = (MAZE_W - 1) * SCALE;
        if (blinky.x >= MAZE_W * SCALE) blinky.x = 0;
    }

    void Draw() override {
        g()->fillScreen(0);
        time_t n; time(&n); struct tm* t = localtime(&n);
        if (t && t->tm_min != lastMin) { if (lastMin != -1) ResetGame(); lastMin = t->tm_min; }
        UpdateMunch(); UpdateBlinky(); DrawMaze(t);
        DrawEntity(munch, (uint16_t)0xFFE0, true);  // Yellow
        DrawEntity(blinky, (uint16_t)0xF800, false); // Red
    }

    void DrawMaze(struct tm* t) {
        const int xOff = 4, yOff = (MATRIX_HEIGHT - MAZE_H) / 2;
        for (int ty = 0; ty < MAZE_H; ty++) {
            uint32_t row = mazeMask[ty];
            for (int tx = 0; tx < MAZE_W; tx++) {
                int px = xOff + (tx * 2);
                bool isHouse = (ty >= kHouseTop && ty <= kHouseBot && tx >= kHouseL && tx <= kHouseR);
                if ((row >> (27 - tx)) & 1) {
                    g()->setPixel(px, yOff + ty, (uint16_t)0x001F); // Blue
                    if (!isHouse) g()->setPixel(px + 1, yOff + ty, (uint16_t)0x001F);
                } else if (!isHouse && pellets.test(ty * MAZE_W + tx) && (tx % 2 == 0) && (ty % 2 == 0)) {
                    g()->setPixel(px + 1, yOff + ty, (uint16_t)0x7BEF); // Gray
                }
            }
        }
        if (t) {
            int h = t->tm_hour % 12; if (h == 0) h = 12;
            int x = (h >= 10) ? 22 : 24; int y = yOff + 12;
            uint16_t cyan = 0x07FF;
            if (h >= 10) { DrawDigit(x, y, 1, cyan); x += 4; }
            DrawDigit(x, y, h % 10, cyan);
            // Colon pulse
            if ((millis() / 500) % 2) {
                g()->setPixel(x + 4, y + 1, (uint16_t)0xFFFF); g()->setPixel(x + 4, y + 3, (uint16_t)0xFFFF);
            }
            DrawDigit(x + 6, y, t->tm_min / 10, cyan); DrawDigit(x + 10, y, t->tm_min % 10, cyan);
        }
    }

    void DrawEntity(MunchEntity& e, uint16_t color, bool animate) {
        int xOff = 4, yOff = (MATRIX_HEIGHT - MAZE_H) / 2;
        int sx = xOff + ((e.x / SCALE) * 2), sy = yOff + (e.y / SCALE);
        bool open = animate && ((millis() / 150) % 2 == 0);
        safeSet(sx, sy, color); safeSet(sx+1, sy, color);
        safeSet(sx, sy-1, color); safeSet(sx+1, sy-1, color);
        safeSet(sx, sy+1, color); safeSet(sx+1, sy+1, color);
        if (!open || e.dir != LEFT)  safeSet(sx-1, sy, color);
        if (!open || e.dir != RIGHT) safeSet(sx+2, sy, color);
        if (!open || e.dir != UP)    { safeSet(sx, sy-1, color); safeSet(sx+1, sy-1, color); }
        if (!open || e.dir != DOWN)  { safeSet(sx, sy+1, color); safeSet(sx+1, sy+1, color); }
    }

    void safeSet(int x, int y, uint16_t c) {
        if (x >= 0 && x < MATRIX_WIDTH && y >= 0 && y < MATRIX_HEIGHT) g()->setPixel(x, y, c);
    }
};
