#pragma once

#include <deque>
#include <vector>
#include <ctime>

#include "globals.h"
#include "ledstripeffect.h"

// Tetris Clock for 64x32 Matrix
// Architecture: "The Assembler"
// Instead of solving bin-packing, we use pre-scripted "Blueprints" for each digit.
// We spawn pieces at the top that are pre-destined to land in the right spot.
// Grid: 64x32 pixels -> 16x8 "Macro Blocks" (4x4 pixels each).

#define TETRIS_GRID_W 16
#define TETRIS_GRID_H 8
#define BLOCK_SIZE 4

// Tetromino Types
enum TetrominoType { T_I=0, T_J, T_L, T_O, T_S, T_T, T_Z, T_NONE };

// Colors (Standard Tetris Colors)
// I=Cyan, J=Blue, L=Orange, O=Yellow, S=Green, T=Purple, Z=Red
// Uses FastLED predefined colors to avoid runtime construction
static const CRGB TETRIS_COLORS[] = {
    CRGB::Cyan,
    CRGB::Blue,
    CRGB::Orange,
    CRGB::Yellow,
    CRGB::Green,
    CRGB::Purple,
    CRGB::Red,
    CRGB::Black // None
};

struct Tetromino {
    TetrominoType type;
    int x;          // Grid X (0-15)
    float y;        // Grid Y (float for smooth gravity)
    int rotation;   // 0-3
    int targetY;    // Where it should stop
    bool settled;   // True if locked in grid
};

struct BlueprintStep {
    TetrominoType type;
    int targetX;
    int targetY;
    int rotation;
};

class PatternCGTetris : public EffectWithId<PatternCGTetris> {
private:
    uint8_t grid[TETRIS_GRID_W][TETRIS_GRID_H]; // Stores ColorIndex + 1 (0=Empty)
    std::vector<Tetromino> fallingPieces;
    int lastMinute = -1;
    
    // State
    enum State { IDLE, CLEARING, SPAWNING };
    State currentState = IDLE;
    int stateTimer = 0;
    
    // Digit Construction Queue
    std::deque<BlueprintStep> buildQueue;

    // Helper: Get shapes (4x2 coordinates for each rotation)
    // We will hardcode these later.
    
    void ClearGrid() {
        memset(grid, 0, sizeof(grid));
    }
    // Optimized bounds check using unsigned cast to catch negative values
    void DrawBlock(int gx, int gy, uint8_t colorIdx) {
        if ((unsigned)gx >= TETRIS_GRID_W || (unsigned)gy >= TETRIS_GRID_H) return;
        // Draw 3x3 block with 1px border (implicit by being 3x3 in 4x4 space? No, 4x4 filled is easiest)
        // Let's do 3x3 filled, top-left aligned, leaving bottom-right gap?
        // Or full 4x4 for CHONKY look? User said 4x4.
        // Use fillRectangle(x0, y0, x1, y1, color) to support CRGB without conversion
        int x = gx * BLOCK_SIZE;
        int y = gy * BLOCK_SIZE;
        int w = BLOCK_SIZE - 1;
        int h = BLOCK_SIZE - 1;
        g()->fillRectangle(x, y, x + w, y + h, TETRIS_COLORS[colorIdx]);
    }

    int currentMinute() {
        time_t now;
        time(&now);
        struct tm *local = localtime(&now);
        return local->tm_min;
    }

public:
    PatternCGTetris() : EffectWithId<PatternCGTetris>("Tetris Clock") {}
    PatternCGTetris(const JsonObjectConst& json) : EffectWithId<PatternCGTetris>(json) {}

    void Start() override {
        ClearGrid();
        fallingPieces.clear();
        currentState = IDLE;
        lastMinute = -1;
    }

    void Draw() override {
        // Time Check
        int min = currentMinute();
        if (min != lastMinute) {
            currentState = CLEARING;
            stateTimer = 0;
            lastMinute = min;
            // TODO: Queue up the new digits here
        }

        // Logic
        switch (currentState) {
            case CLEARING:
                // TODO: Line clear animation on old digits
                if (++stateTimer > 30) {
                    ClearGrid();
                    currentState = SPAWNING;
                    // Mockup: Queue a '1' just to see it work
                    // BuildQueue: I-Piece at x=2, y=6
                    // buildQueue.push_back({T_I, 2, 6, 0}); 
                }
                break;
                
            case SPAWNING:
                // Drop one piece every N frames
                if (stateTimer++ % 20 == 0 && !buildQueue.empty()) {
                    BlueprintStep step = buildQueue.front();
                    buildQueue.pop_front();
                    Tetromino newPiece = { step.type, step.targetX, -4.0f, step.rotation, step.targetY, false };
                    fallingPieces.push_back(newPiece);
                }
                if (fallingPieces.empty() && buildQueue.empty()) currentState = IDLE;
                break;
                
            case IDLE:
                break;
        }

        // Physics (Gravity)
        for (auto& p : fallingPieces) {
            if (p.settled) continue;
            
            p.y += 0.25f; // fast fall
            
            // Hard landing (Fake collision)
            if (p.y >= p.targetY) {
                p.y = (float)p.targetY;
                p.settled = true;
                // Lock into grid (simplified: just 1 block for now to test rendering)
                // Real Tetris needs to lock 4 blocks based on shape
                grid[p.x][p.targetY] = p.type + 1; // Hack for single block test
            }
        }

        // Render
        g()->fillScreen(0); // Black background
        
        // Draw Grid
        for (int x = 0; x < TETRIS_GRID_W; x++) {
            for (int y = 0; y < TETRIS_GRID_H; y++) {
                if (grid[x][y] > 0) {
                    DrawBlock(x, y, grid[x][y] - 1);
                }
            }
        }
        
        // Draw Falling
        for (const auto& p : fallingPieces) {
            if (!p.settled) {
                DrawBlock(p.x, (int)p.y, p.type); // Hack: Drawing just pivot for now
            }
        }
        
        // Blink Colon (Cols 7 & 8?)
        if (millis() % 1000 < 500) {
            // gfx->fillRect...
        }
    }
};
