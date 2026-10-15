#pragma once

#include <array>
#include <cstdint>

namespace pong {

struct Vec2 {
    float x = 0.f;
    float y = 0.f;
};

struct Ball {
    Vec2 position{};
    Vec2 velocity{};
    float radius = 8.f;
};

struct Paddle {
    Vec2 position{};
    float width = 12.f;
    float height = 100.f;
    float speed = 420.f;
};

struct GameState {
    Ball ball{};
    Paddle leftPaddle{};
    Paddle rightPaddle{};
    std::int32_t leftScore = 0;
    std::int32_t rightScore = 0;
    std::int32_t scoreLimit = 5;
    std::int32_t leftRoundScore = 0;
    std::int32_t rightRoundScore = 0;
    std::int32_t roundScoreLimit = 2;
    std::int32_t roundsPlayed = 0;
    std::array<std::int32_t, 3> roundWinners = {0, 0, 0}; // 0 none, 1 left, 2 right
    bool gameOver = false;
    bool leftWon = false;
    bool matchOver = false;
};

} // namespace pong
