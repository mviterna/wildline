# Knobs State Forest

Select **Maps > Knobs State Forest**, then enter the waiting room. The map uses
Classic Hunt rules, the shared Classic match controls, a hollow den and 30 acorns.
It can also be selected under Classic's layout setting. Both team spawns, the den,
terrain collision and climbable wood use the new terrain height field.

The roughly 290 m wide play area compresses the real forest into three connected
trail loops, a creek valley, rounded ridges, grass openings and three trailheads.
Crooked Creek has gravel beds and isolated shallow pools. Trails are open to both
roles; blue/orange/yellow markers identify the loops and red marks the boundary.

Authoring:

1. Build the editor module.
2. Run `Scripts/import_knobs_forest.py` through UnrealEditor-Cmd with
   `-ExecutePythonScript=... -unattended -NullRHI -NoSound`.
3. Run `native/CaptureMapPreviews.ps1 -Maps KnobsStateForest`.
4. Package through `native/LaunchNative.ps1 -Build`.
5. Run `native/VerifyKnobsPackage.ps1 -Capture -Network`.

`WildlineKnobs.h` owns the shared deterministic layout and terrain.
`WildlineKnobs.cpp` authors trails, creek and landmarks; `WildlineWorld.cpp`
plants the retained forest assets with clear trail corridors. Runtime evidence
and captures are written to `Review/`. See `PROVENANCE.md` for references.
