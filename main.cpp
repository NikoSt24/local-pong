// =============================================================================
// Stage 1 — local Pong (SFML 3)
// =============================================================================
//
// HOW THE LOOP FITS TOGETHER (read this once, then skim the labels in main):
//
//   1) pollEvent     — window close, focus, key pressed/released → updates HeldKeys
//   2) accum += dt   — add this frame’s real time to the “accumulator” (see below)
//   3) while accum   — run physics in fixed steps (e.g. 1/60 s), not “whenever the GPU feels like it”
//   4) clear/draw    — show the latest state (can run at a different rate than physics)
//
// WHY FIXED TIMESTEP?
//   Real frames take variable time (16 ms, 9 ms, 22 ms…). If you move the ball with
//   position += velocity * (variable frame time), speed changes when FPS drops.
//   So we step the simulation with a constant dt (kFixedDtSec) and only use real time
//   to know *how many* of those steps to run this frame.
//
// WHAT IS accum?
//   Short for “accumulator”: seconds of real time **not yet consumed** by physics steps.
//   Each frame: accum += time_since_last_frame
//   Then: while (accum >= one_physics_step) { update(one_physics_step); accum -= step; }
//   So you might run 0, 1, or 2+ physics updates in a single frame — that’s normal.
//   The cap (0.25 s) avoids a huge catch-up spike after pausing in the debugger.
//
// WHAT YOU CAN ADD NEXT (rough order):
//   — Proper ball–paddle collision (circle vs rectangle; your current checks mix center/edges)
//   — Score when ball passes left/right **goal line** (x < 0 or x > field width)
//   — resetRound() after a point; checkWin() when score reaches scoreLimit
//   — sf::Text + font for on-screen score; optional “Press R to restart”
//   — Split into game.hpp / game.cpp / collision.cpp when this file feels crowded
//
// =============================================================================

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include <SFML/Window.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "game_types.hpp"

namespace {

// --- Logical playfield (all physics uses these units; the View maps them to pixels) ---
constexpr float kFieldW = 800.f;
constexpr float kFieldH = 600.f;
const sf::Color kLeftColor(100, 180, 255);
const sf::Color kRightColor(255, 160, 100);

// One simulation step = 1/60 second. update() should always receive THIS value, not variable frame time.
constexpr float kFixedDtSec = 1.f / 60.f;

// ---------------------------------------------------------------------------
// Input: two ways combined so movement feels reliable on macOS
// ---------------------------------------------------------------------------
// 1) HeldKeys — toggled by KeyPressed / KeyReleased (good when repeat is off).
// 2) isKeyPressed — real-time “is this key down right now?”
//    The window must have keyboard focus (click the game). FocusLost clears HeldKeys.
// ---------------------------------------------------------------------------

struct HeldKeys {
    bool w = false;
    bool s = false;
    bool up = false;
    bool down = false;
};

static void apply_key_code(sf::Keyboard::Scancode code, bool pressed, HeldKeys& held) {
    switch (code) {
        case sf::Keyboard::Scancode::W:
            held.w = pressed;
            break;
        case sf::Keyboard::Scancode::S:
            held.s = pressed;
            break;
        case sf::Keyboard::Scancode::Up:
            held.up = pressed;
            break;
        case sf::Keyboard::Scancode::Down:
            held.down = pressed;
            break;
        default:
            break;
    }
}

// Fills the four bools that update() uses for paddle movement (left: W/S, right: arrows).
static void sample_input(const HeldKeys& held, bool& leftUp, bool& leftDown, bool& rightUp, bool& rightDown) {
    leftUp = held.w;
    leftDown = held.s;
    rightUp = held.up;
    rightDown = held.down;
}

// ---------------------------------------------------------------------------
// render — read-only: turns GameState into rectangles/circle (SFML drawables)
// ---------------------------------------------------------------------------
// YOUR NEXT: add sf::Text for scores; maybe a dim overlay when gameOver is true.
// ---------------------------------------------------------------------------

// `font` may be nullptr if no font on disk could be loaded; in that case we just skip the score text.
// IMPORTANT: the font must NOT be a function-local static or a global. SFML graphics objects must be
// destroyed BEFORE the sf::RenderWindow (which owns the OpenGL context). If the font outlives the
// window, its destructor crashes the program at exit. So we keep it owned by main().
static void render(sf::RenderWindow& window, const pong::GameState& state, const sf::Font* font) {
    window.setView(sf::View(sf::FloatRect({0.f, 0.f}, {kFieldW, kFieldH})));

    if (state.matchOver) {
        sf::RectangleShape endScreen({kFieldW, kFieldH});
        endScreen.setFillColor(sf::Color::Black);
        window.draw(endScreen);
        if (font != nullptr) {
            const bool leftWonMatch = state.leftRoundScore > state.rightRoundScore;
            sf::Text over(*font,
                          sf::String(leftWonMatch ? "Match Over: Left Side Wins"
                                                  : "Match Over: Right Side Wins"),
                          40u);
            over.setFillColor(sf::Color::White);
            over.setPosition({kFieldW * 0.5f - 235.f, kFieldH * 0.5f - 20.f});
            window.draw(over);
        }
        return;
    }

    sf::RectangleShape centerLine({2.f, kFieldH});
    centerLine.setPosition({kFieldW * 0.5f - 1.f, 0.f});
    centerLine.setFillColor(sf::Color(70, 70, 80));
    window.draw(centerLine);

    sf::RectangleShape leftRect({state.leftPaddle.width, state.leftPaddle.height});
    leftRect.setOrigin({state.leftPaddle.width * 0.5f, state.leftPaddle.height * 0.5f});
    leftRect.setPosition({state.leftPaddle.position.x, state.leftPaddle.position.y});
    leftRect.setFillColor(kLeftColor);
    window.draw(leftRect);

    sf::RectangleShape rightRect({state.rightPaddle.width, state.rightPaddle.height});
    rightRect.setOrigin({state.rightPaddle.width * 0.5f, state.rightPaddle.height * 0.5f});
    rightRect.setPosition({state.rightPaddle.position.x, state.rightPaddle.position.y});
    rightRect.setFillColor(kRightColor);
    window.draw(rightRect);

    sf::CircleShape ballShape(state.ball.radius);
    ballShape.setOrigin({state.ball.radius, state.ball.radius});
    ballShape.setPosition({state.ball.position.x, state.ball.position.y});
    ballShape.setFillColor(sf::Color::White);
    window.draw(ballShape);

    if (font != nullptr) {
        char buf[48];
        std::snprintf(buf, sizeof(buf), "%d  —  %d", static_cast<int>(state.leftScore),
                      static_cast<int>(state.rightScore));
        sf::Text scores(*font, sf::String(buf), 28u);
        scores.setFillColor(sf::Color::White);
        scores.setPosition({kFieldW * 0.5f - 48.f, 14.f});
        window.draw(scores);

        sf::Text rounds(*font,
                        sf::String("Rounds " + std::to_string(state.leftRoundScore) + " - " +
                                   std::to_string(state.rightRoundScore)),
                        16u);
        rounds.setFillColor(sf::Color(180, 180, 190));
        rounds.setPosition({kFieldW * 0.5f - 56.f, 46.f});
        window.draw(rounds);
    }

    // Round markers (best-of-3): fill these three top squares by round winner.
    constexpr float kSquareSize = 18.f;
    constexpr float kSquareGap = 8.f;
    const float totalWidth = 3.f * kSquareSize + 2.f * kSquareGap;
    const float startX = kFieldW * 0.5f - totalWidth * 0.5f;
    for (int i = 0; i < 3; ++i) {
        sf::RectangleShape square({kSquareSize, kSquareSize});
        square.setPosition({startX + i * (kSquareSize + kSquareGap), 70.f});
        square.setOutlineColor(sf::Color(160, 160, 160));
        square.setOutlineThickness(1.5f);
        const std::int32_t winner = state.roundWinners[static_cast<std::size_t>(i)];
        if (winner == 1) {
            square.setFillColor(kLeftColor);
        } else if (winner == 2) {
            square.setFillColor(kRightColor);
        } else {
            square.setFillColor(sf::Color(45, 45, 55));
        }
        window.draw(square);
    }
}

// Tries each candidate path in order; returns true on first success. Caller owns the font.
static bool loadScoreFont(sf::Font& font) {
    const char* kFontPaths[] = {
        "assets/fonts/DejaVuSans.ttf",
        "/System/Library/Fonts/Supplemental/Arial.ttf",
        "/Library/Fonts/Arial.ttf",
    };
    for (const char* path : kFontPaths) {
        if (font.openFromFile(path)) {
            return true;
        }
    }
    return false;
}

static float speed_up_preserving_direction(float v) {
    constexpr float kSpeedStep = 25.f;
    constexpr float kMaxSpeed = 600.f;
    const float direction = (v < 0.f) ? -1.f : 1.f;
    const float nextMagnitude = std::min(std::abs(v) + kSpeedStep, kMaxSpeed);
    return direction * nextMagnitude;
}

static void updateBallAfterHit(pong::GameState& state) {
    state.ball.velocity.x = speed_up_preserving_direction(state.ball.velocity.x);
    state.ball.velocity.y = speed_up_preserving_direction(state.ball.velocity.y);
}

static float clampf(float v, float minV, float maxV) {
    return std::max(minV, std::min(v, maxV));
}

static bool circleIntersectsPaddle(const pong::Ball& ball, const pong::Paddle& paddle) {
    const float left = paddle.position.x - paddle.width * 0.5f;
    const float right = paddle.position.x + paddle.width * 0.5f;
    const float top = paddle.position.y - paddle.height * 0.5f;
    const float bottom = paddle.position.y + paddle.height * 0.5f;
    const float closestX = clampf(ball.position.x, left, right);
    const float closestY = clampf(ball.position.y, top, bottom);
    const float dx = ball.position.x - closestX;
    const float dy = ball.position.y - closestY;
    return (dx * dx + dy * dy) <= (ball.radius * ball.radius);
}

static void resetBall(pong::GameState& state) {
    state.ball.position = {kFieldW * 0.5f, kFieldH * 0.5f};
    state.ball.velocity = {240.f, 140.f};
    state.ball.radius = 8.f;
}


static void resetRound(pong::GameState& state) {
    resetBall(state);
    state.leftPaddle.position = {40.f, kFieldH * 0.5f};
    state.rightPaddle.position = {kFieldW - 40.f, kFieldH * 0.5f};
    state.leftWon = false;
    state.gameOver = false;
    state.leftScore = 0;
    state.rightScore = 0;
}

static void checkWin(pong::GameState& state) {
    if (state.leftScore >= state.scoreLimit) {
        state.leftWon = true;
        state.gameOver = true;
    } else if (state.rightScore >= state.scoreLimit) {
        state.leftWon = false;
        state.gameOver = true;
    }
}

// ---------------------------------------------------------------------------
// update — simulation only: move paddles, integrate ball, collisions, (later) score
// ---------------------------------------------------------------------------
// YOUR NEXT:
//   — Replace the simple paddle “if” blocks with real circle-vs-AABB collision + bounce angle.
//   — When ball x passes left/right goal, increment score, reset ball, printf/log.
//   — When leftScore or rightScore >= scoreLimit, set gameOver, leftWon, stop play.
// ---------------------------------------------------------------------------

static void update(float dt, pong::GameState& state, bool leftUp, bool leftDown, bool rightUp, bool rightDown) {
    if (state.matchOver || state.gameOver) {
        return;
    }
    

    auto movePaddle = [&](pong::Paddle& p, bool up, bool down) {
        float dy = 0.f;
        if (up) {
            dy -= 1.f;
        }
        if (down) {
            dy += 1.f;
        }
        p.position.y += dy * p.speed * dt;
        const float half = p.height * 0.5f;
        if (p.position.y < half) {
            p.position.y = half;
        }
        if (p.position.y > kFieldH - half) {
            p.position.y = kFieldH - half;
        }
    };

    movePaddle(state.leftPaddle, leftUp, leftDown);
    movePaddle(state.rightPaddle, rightUp, rightDown);

    state.ball.position.x += state.ball.velocity.x * dt;
    state.ball.position.y += state.ball.velocity.y * dt;

    // Top / bottom walls
    if (state.ball.position.y - state.ball.radius < 0.f) {
        state.ball.velocity.y = -state.ball.velocity.y;
        state.ball.position.y = state.ball.radius;
    } else if (state.ball.position.y + state.ball.radius > kFieldH) {
        state.ball.velocity.y = -state.ball.velocity.y;
        state.ball.position.y = kFieldH - state.ball.radius;
    }

    // Precise circle-vs-rectangle collision against paddle bounds.
    if (circleIntersectsPaddle(state.ball, state.leftPaddle) && state.ball.velocity.x < 0.f) {
        state.ball.velocity.x = -state.ball.velocity.x;
        state.ball.position.x =
            state.leftPaddle.position.x + state.leftPaddle.width * 0.5f + state.ball.radius;
        updateBallAfterHit(state);
    }
    if (circleIntersectsPaddle(state.ball, state.rightPaddle) && state.ball.velocity.x > 0.f) {
        state.ball.velocity.x = -state.ball.velocity.x;
        state.ball.position.x =
            state.rightPaddle.position.x - state.rightPaddle.width * 0.5f - state.ball.radius;
        updateBallAfterHit(state);
    }

    // TODO: goals (ball past x), scoring, resetRound, checkWin — see top block comment.
    if (state.ball.position.x > kFieldW) {
        state.leftScore++;
        resetBall(state);
        checkWin(state);
        if (state.gameOver) {
            state.leftRoundScore++;
            if (state.roundsPlayed < static_cast<std::int32_t>(state.roundWinners.size())) {
                state.roundWinners[static_cast<std::size_t>(state.roundsPlayed)] = 1;
            }
            state.roundsPlayed++;
            if (state.leftRoundScore >= state.roundScoreLimit || state.roundsPlayed >= 3) {
                state.matchOver = true;
            } else {
                resetRound(state);
            }
        }
    } else if (state.ball.position.x < 0.f) {
        state.rightScore++;
        resetBall(state);
        checkWin(state);
        if (state.gameOver) {
            state.rightRoundScore++;
            if (state.roundsPlayed < static_cast<std::int32_t>(state.roundWinners.size())) {
                state.roundWinners[static_cast<std::size_t>(state.roundsPlayed)] = 2;
            }
            state.roundsPlayed++;
            if (state.rightRoundScore >= state.roundScoreLimit || state.roundsPlayed >= 3) {
                state.matchOver = true;
            } else {
                resetRound(state);
            }
        }
    }
}

} // namespace

int main() {
    sf::RenderWindow window(sf::VideoMode({900u, 600u}), "Pong");
    window.setKeyRepeatEnabled(false);
    window.requestFocus();

    // Font is owned here, AFTER the window. C++ destroys locals in reverse declaration order, so
    // `scoreFont` dies BEFORE `window` — which is required so SFML can still talk to OpenGL during
    // its destructor. Don't move this to global / function-static scope.
    sf::Font scoreFont;
    const bool fontLoaded = loadScoreFont(scoreFont);

    HeldKeys held{};
    pong::GameState state{};
    state.ball.position = {kFieldW * 0.5f, kFieldH * 0.5f};
    state.ball.velocity = {240.f, 140.f};
    state.ball.radius = 8.f;
    state.leftPaddle.position = {40.f, kFieldH * 0.5f};
    state.rightPaddle.position = {kFieldW - 40.f, kFieldH * 0.5f};
    state.leftWon = false;

    sf::Clock clock;
    // accum = seconds of real time waiting to be “spent” on fixed-size physics steps.
    float accum = 0.f;

    while (window.isOpen()) {
        // --- Events: drive HeldKeys (and close window) ---
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (event->is<sf::Event::FocusLost>()) {
                held = {};
            } else if (const auto* kp = event->getIf<sf::Event::KeyPressed>()) {
                apply_key_code(kp->scancode, true, held);
            } else if (const auto* kr = event->getIf<sf::Event::KeyReleased>()) {
                apply_key_code(kr->scancode, false, held);
            }
        }

        bool lu = false, ld = false, ru = false, rd = false;

        // --- Fixed timestep: add frame duration to accum, then drain it in 1/60 s chunks ---
        accum += clock.restart().asSeconds();
        const float kMaxAccumSec = 0.25f; // avoid spiral of death after a hang
        if (accum > kMaxAccumSec) {
            accum = kMaxAccumSec;
        }
        while (accum >= kFixedDtSec) {
            sample_input(held, lu, ld, ru, rd);
            update(kFixedDtSec, state, lu, ld, ru, rd);
            accum -= kFixedDtSec;
        }

        // --- Draw current state (no physics here) ---
        window.clear(sf::Color(30, 30, 40));
        render(window, state, fontLoaded ? &scoreFont : nullptr);
        window.display();
    }

    return 0;
}
