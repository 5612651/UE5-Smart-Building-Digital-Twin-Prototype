# UE5 Smart Building Digital Twin Prototype

> Personal portfolio prototype · Unreal Engine 5.8 · C++ · UMG · CSV-driven building data

A C++-driven Unreal Engine prototype that explores how a smart-building digital twin can combine 3D spatial interaction, room-level BMS data, environmental visualization, and an operator-facing dashboard.

This is **not** a deployed building system and does not use real institutional or live BMS data. All monitoring data, alert scenarios, and room semantics are simulated for demonstration purposes.

## Try the Demo

Download the latest Windows build from the [Releases page](https://github.com/5612651/UE5-Smart-Building-Digital-Twin-Prototype/releases/latest), extract the ZIP, and run `Building.exe`.

No Unreal Engine installation is required for the packaged demo. Keep the entire extracted `Windows` directory intact; `Building.exe` depends on the adjacent content and runtime files.

## What This Prototype Demonstrates

- C++-controlled orbit, overview, and room-camera modes.
- Clickable Room001–Room026 volumes that switch to the matching room camera.
- Automatic orbit rotation with manual mouse rotation and scroll-wheel zoom.
- A spatial temperature layer that remains visible through the building envelope in orbit and overview modes.
- CSV-backed room data for temperature, humidity, CO₂, occupancy, energy, HVAC load/mode, and fresh-air ratio.
- A two-year hourly simulation timeline (2026–2027) with classroom schedules, workdays, holidays, vacations, and intentionally injected abnormal conditions.
- Alert-first building status view and room-level real-time data panels.
- Simulated day/night lighting, including room point lights and time-based sun/moon behavior.
- Responsive UMG dashboard UI, room selector, timeline controls, demo presets, and in-app exit control.

## Architecture

| Area | Main implementation |
| --- | --- |
| Digital-twin state | `ADigitalTwinManager` manages view mode, selected room, simulated time, overlays, and lighting. |
| Interaction | `ADTDigitalTwinPlayerController` routes UI and scene input, including spatial room selection. |
| Cameras | `ADTOrbitCamera` provides automated/manual orbit control; `ADTRoomCamera` serves the overview and 26 room views. |
| Data | `UDigitalTwinDataSubsystem` loads and caches the CSV data and metadata. |
| UI | `UDigitalTwinControlWidget` builds and updates the dashboard through native C++/UMG logic. |
| Level | UE World Partition with external actors, including the room visualization volumes. |

## Project Layout

```text
Source/Building/       Native C++ gameplay, cameras, UI, manager, and data subsystem
Content/Untitled/      Main World Partition level and its external actors
Content/Digital_Twins/ Building model, materials, textures, and lighting assets
Content/Data/          Room metadata plus hourly BMS simulation CSV files
Content/DigitalTwin/   Runtime spatial-overlay material
Config/                Input, game mode, map, and packaging configuration
Scripts/               Reproducible editor-side asset configuration utilities
```

## Open the Source Project

Requirements: Unreal Engine 5.8 and Visual Studio with the C++ game-development workload.

1. Clone the repository.
2. Open `Building.sln` and build `BuildingEditor Win64 Development`.
3. Open `Building.uproject`.
4. Start `/Game/Untitled` using PIE or Standalone Game.

The project is configured to use the native `DTDigitalTwinGameMode` and start from `/Game/Untitled`.

## Data Disclaimer

`Content/Data/BMSHourly/` contains synthetic hourly data for visualization and interaction testing only. It should not be interpreted as real building performance, occupancy, or environmental records.

## Portfolio Note

The source repository intentionally excludes packaged binaries, Unreal cache folders, intermediate build output, and IDE caches. The ready-to-run Windows build is distributed through GitHub Releases instead.
