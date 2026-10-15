# Explaining this project

Before describing the upgrade as your work, run it, read the updated source and make a small change yourself. The update used AI assistance; be ready to explain what you verified and what you understand.

## A 60-second demo

1. Show the original and updated source side by side.
2. Start solo mode and show the three difficulty choices.
3. Pause, restart and explain that these are explicit game states.
4. Run CTest and open the high-speed collision check.

## Questions to practice

**Why a fixed timestep?** Physics receives the same small time increment even when rendering varies. The accumulator runs as many fixed steps as needed, with catch-up capped after a stall. Rendering is not interpolated, so there can still be subtle visual stepping.

**What is tunnelling?** A fast object can cross a thin paddle between two snapshots. The code checks whether its motion segment crossed the paddle face and estimates the impact time. Read the `collide` lambda in `Game::tick`.

**How does difficulty work?** Easier AI reacts less often, moves slower and has larger aim error. It is a rule-based opponent, not machine learning.

**Why separate logic from graphics?** Tests can exercise scoring and physics without creating a window or installing SFML. This also makes it easier to replace the renderer later.

**What would you improve next?** Add more boundary/corner tests, playtest AI balance with real players, add rendering interpolation and ship a portable package. Exact circle-to-corner collision is a known simplification.

## Small changes to make yourself

- Change the score required for a round, update the checks, and explain the failing tests before fixing them.
- Tune the AI reaction delay and compare how it feels.
- Add a test showing that the simulation remains still after the match ends.

Be candid if asked about tools: "I built the initial prototype at a hackathon, then used AI assistance to expand and test it. Here is how the update works and what I checked." Only claim that final verification yourself after doing it.
