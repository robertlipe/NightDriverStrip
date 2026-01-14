#pragma once

#include <vector>
#include <chrono>
#include <algorithm>
#include <cmath>

struct Brick {
    float x, y;
    uint32_t color;
    bool active;
};

struct Ball {
    float x, y;
    float vx, vy;
};

struct Paddle {
    float x;
};

class PatternCGBreakout : public EffectWithId<PatternCGBreakout> {
private:
    // Named Constants
    static constexpr float kBallSpeed = 1.2f;
    static constexpr float kPaddleSpeed = 1.5f;
    static constexpr float kPaddleWidth = 8.0f;
    static constexpr float kPaddleY = 28.0f;
    static constexpr float kBallRadius = 0.5f;
    static constexpr float kBrickWidth = 1.0f;
    static constexpr float kBrickHeight = 1.0f;
    static constexpr unsigned int kMissChance = 10; // 10% chance to miss

    std::vector<Brick> bricks;
    Ball ball;
    Paddle paddle;
    int lastMin = -1;
    uint32_t lastMissCheck = 0;
    bool shouldMiss = false;

    static constexpr uint32_t font3x5[11][5] = {
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
    PatternCGBreakout() : EffectWithId<PatternCGBreakout>("CG Breakout") { ResetGame(); }
    PatternCGBreakout(const JsonObjectConst& json) : EffectWithId<PatternCGBreakout>(json) { ResetGame(); }

    void Start() override { ResetGame(); }

    void ResetGame() {
        bricks.clear();
        paddle.x = (float)MATRIX_WIDTH / 2.0f;
        ball.x = (float)MATRIX_WIDTH / 2.0f;
        ball.y = 20.0f;
        ball.vx = 0.8f;
        ball.vy = -1.0f;
        // Normalize ball velocity
        float speed = sqrtf(ball.vx * ball.vx + ball.vy * ball.vy);
        ball.vx = (ball.vx / speed) * kBallSpeed;
        ball.vy = (ball.vy / speed) * kBallSpeed;
        shouldMiss = false;
        SpawnTimeBricks();
    }

    void SpawnTimeBricks() {
        bricks.clear();
        auto now_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        struct tm tm_buf;
        struct tm* t = localtime_r(&now_t, &tm_buf);
        lastMin = t->tm_min;
        
        // Classic Breakout colors: Red, Orange, Yellow, Green, Cyan, Blue
        static constexpr uint32_t breakoutColors[] = {
            (uint32_t)CRGB::Red,
            (uint32_t)CRGB::OrangeRed,
            (uint32_t)CRGB::Yellow,
            (uint32_t)CRGB::Green,
            (uint32_t)CRGB::Cyan,
            (uint32_t)CRGB::Blue
        };

        // Brick dimensions (classic Breakout style)
        static constexpr unsigned int kBrickWidth = 5;  // 5 pixels wide
        static constexpr unsigned int kBrickGap = 1;    // 1 pixel gap
        static constexpr unsigned int kBrickHeight = 2; // 2 pixels tall

        // Add time digits at the very top (like Pong score)
        unsigned int digits[5] = {
            (unsigned int)(t->tm_hour/10), (unsigned int)(t->tm_hour%10), 10,
            (unsigned int)(t->tm_min/10), (unsigned int)(t->tm_min%10)
        };
        unsigned int startX = (MATRIX_WIDTH - (5 * 4)) / 2;
        unsigned int timeY = 0; // Time at very top

        for (unsigned int d = 0; d < 5; d++) {
            for (unsigned int row = 0; row < 5; row++) {
                uint32_t rowBits = font3x5[digits[d]][row];
                for (unsigned int col = 0; col < 3; col++) {
                    if ((rowBits >> (2 - col)) & 1) {
                        float px = startX + (d * 4) + col;
                        float py = timeY + row;
                        uint32_t color = (d < 2) ? (uint32_t)CRGB::Cyan : 
                                        (d == 2) ? (uint32_t)CRGB::White : 
                                        (uint32_t)CRGB::Green;
                        bricks.push_back({px, py, color, true});
                    }
                }
            }
        }

        // Add decorative brick rows below time
        unsigned int brickStartY = 6; // Start bricks below time
        for (unsigned int row = 0; row < 4; row++) {
            uint32_t rowColor = breakoutColors[row % 6];
            unsigned int yBase = brickStartY + (row * (kBrickHeight + 1));
            if (yBase < kPaddleY - 5) {
                for (unsigned int bx = 0; bx < MATRIX_WIDTH; bx += (kBrickWidth + kBrickGap)) {
                    for (unsigned int by = 0; by < kBrickHeight; by++) {
                        for (unsigned int px = 0; px < kBrickWidth && (bx + px) < MATRIX_WIDTH; px++) {
                            float y = yBase + by;
                            if (y < kPaddleY - 5) {
                                bricks.push_back({(float)(bx + px), y, rowColor, true});
                            }
                        }
                    }
                }
            }
        }
    }

    void Draw() override {
        fadeAllChannelsToBlackBy(40);

        // Check for minute change
        auto now_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        struct tm tm_buf;
        if (localtime_r(&now_t, &tm_buf)->tm_min != lastMin) {
            SpawnTimeBricks();
        }

        UpdateBall();
        UpdatePaddle();

        // Draw bricks
        for (const auto& brick : bricks) {
            if (brick.active) {
                safeSet(brick.x, brick.y, CRGB(brick.color));
            }
        }

        // Draw ball
        safeSet(ball.x, ball.y, CRGB::White);

        // Draw paddle
        int px = lroundf(paddle.x);
        for (int i = -4; i <= 3; i++) {
            safeSet(px + i, kPaddleY, CRGB::Red);
        }
    }

    void UpdateBall() {
        ball.x += ball.vx;
        ball.y += ball.vy;

        // Check if ball is too horizontal (boring trajectory)
        // If |vy| is very small compared to |vx|, respawn the ball
        float slope = fabsf(ball.vy / ball.vx);
        if (slope < 0.3f) { // Slope less than ~17 degrees
            // Ball is too horizontal, force respawn
            ball.x = (float)MATRIX_WIDTH / 2.0f;
            ball.y = 20.0f;
            ball.vx = ((float)random(100) - 50.0f) / 50.0f;
            ball.vy = -1.0f;
            float speed = sqrtf(ball.vx * ball.vx + ball.vy * ball.vy);
            ball.vx = (ball.vx / speed) * kBallSpeed;
            ball.vy = (ball.vy / speed) * kBallSpeed;
            return;
        }

        // Wall collisions
        if (ball.x <= 0 || ball.x >= MATRIX_WIDTH - 1) {
            ball.vx = -ball.vx;
            ball.x = std::clamp(ball.x, 0.0f, (float)(MATRIX_WIDTH - 1));
        }
        if (ball.y <= 0) {
            ball.vy = -ball.vy;
            ball.y = 0;
        }

        // Paddle collision
        if (ball.y >= kPaddleY - 1 && ball.y <= kPaddleY + 1) {
            if (fabsf(ball.x - paddle.x) <= kPaddleWidth / 2.0f) {
                ball.vy = -fabsf(ball.vy); // Always bounce up
                // Add angle based on hit position
                float offset = (ball.x - paddle.x) / (kPaddleWidth / 2.0f);
                ball.vx += offset * 0.5f;
                // Renormalize
                float speed = sqrtf(ball.vx * ball.vx + ball.vy * ball.vy);
                ball.vx = (ball.vx / speed) * kBallSpeed;
                ball.vy = (ball.vy / speed) * kBallSpeed;
            }
        }

        // Brick collisions
        for (auto& brick : bricks) {
            if (!brick.active) continue;
            if (fabsf(ball.x - brick.x) < kBrickWidth && 
                fabsf(ball.y - brick.y) < kBrickHeight) {
                brick.active = false;
                ball.vy = -ball.vy;
                break;
            }
        }

        // Ball missed - respawn
        if (ball.y > MATRIX_HEIGHT) {
            ball.x = (float)MATRIX_WIDTH / 2.0f;
            ball.y = 20.0f;
            ball.vx = ((float)random(100) - 50.0f) / 50.0f;
            ball.vy = -1.0f;
            float speed = sqrtf(ball.vx * ball.vx + ball.vy * ball.vy);
            ball.vx = (ball.vx / speed) * kBallSpeed;
            ball.vy = (ball.vy / speed) * kBallSpeed;
        }
    }

    void UpdatePaddle() {
        // Decide if we should miss this ball (10% chance, check every second)
        if (millis() - lastMissCheck > 1000) {
            shouldMiss = (random(100) < kMissChance);
            lastMissCheck = millis();
        }

        if (!shouldMiss && ball.vy > 0) { // Only track if ball is moving down
            // Add intentional error to paddle tracking
            float error = ((float)random(100) - 50.0f) / 25.0f; // -2 to +2
            float targetX = ball.x + error;
            
            // Move paddle toward target
            float diff = targetX - paddle.x;
            if (fabsf(diff) > 0.5f) {
                paddle.x += std::clamp(diff, -kPaddleSpeed, kPaddleSpeed);
            }
        }
        
        paddle.x = std::clamp(paddle.x, kPaddleWidth / 2.0f, 
                             (float)MATRIX_WIDTH - kPaddleWidth / 2.0f);
    }

    void safeSet(float x, float y, CRGB c) const {
        int ix = lroundf(x);
        int iy = lroundf(y);
        if ((unsigned int)ix < MATRIX_WIDTH && (unsigned int)iy < MATRIX_HEIGHT) {
            g()->setPixel(ix, iy, c);
        }
    }
};
