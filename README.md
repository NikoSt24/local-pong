# Local Pong

A local two-player Pong game written in C++17 with SFML 3 and built with CMake.

## Controls and rules

- Left player: W and S.
- Right player: Up and Down arrows.
- First to five points wins a round; first to two rounds wins the match.
- Close and reopen the game to start a new match.

## Implementation

The simulation runs at a fixed 60 Hz using a time accumulator, separately from rendering. Catch-up time is capped at 0.25 seconds. Circle-to-rectangle intersection checks handle paddle collisions. Direction checks prevent repeated rebounds while the ball is moving away from a paddle. Paddle hits increase ball velocity, capped at 600 units per second on each axis. Game state tracks points, round winners, and the match winner.

## Build and run

Requires CMake 3.24 or later, a C++17 compiler, and SFML 3. On macOS with Homebrew:

```sh
brew install cmake sfml
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/pong
```

Run the executable from the repository root. The macOS build has been verified. Linux and Windows have not been tested; executable paths may differ.

## Files

- `main.cpp`: input handling, simulation, collision detection, scoring, and rendering.
- `game_types.hpp`: ball, paddle, and game-state data structures.
- `CMakeLists.txt`: executable and SFML dependency configuration.

## Fonts

The game first looks for `assets/fonts/DejaVuSans.ttf`, then tries macOS Arial paths. A font is not redistributed here. On macOS, the system font fallback displays scores and the match-winner message. On other systems, provide a font you are permitted to use at the asset path. Gameplay continues without text if no font can be loaded.

## Current limits

Two players share one keyboard. Online play, an in-game restart menu, and packaged installers are not implemented. Building successfully does not establish that every gameplay path is bug-free.
