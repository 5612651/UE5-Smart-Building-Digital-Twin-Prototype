# UE5 Smart Building Digital Twin Prototype

A personal portfolio prototype built with Unreal Engine 5.8 and C++ to explore smart-building digital twin visualization and interaction.

This project is not based on a real deployed building or live BMS system. It uses a 3D building model and simulated building-management data to demonstrate how room-level environmental information, energy data, spatial visualization, and interactive monitoring could be integrated into a digital twin interface.

## Highlights

- Three presentation modes: orbit view, building overview, and room view.
- Clickable Room001–Room026 spatial volumes; room volumes are hidden automatically in room view.
- C++ camera and interaction layer with automatic orbit rotation and manual mouse control.
- Two-year hourly BMS simulation dataset (2026-01-01 to 2027-12-31), including class schedules, workdays, holidays, vacations, and demonstration anomalies.
- Room-level environmental and operational metrics: temperature, humidity, CO2, occupancy, energy use, HVAC state, and ventilation ratio.
- Translucent temperature overlay that remains visible through the building envelope in orbit/overview modes.
- Time playback, date/hour controls, day/night sun and moon behavior, and room point lights.
- Dashboard UI with alert-centric building status and room-level real-time data cards.
- In-app `退出程序` button for packaged demonstrations.

## Technology

- Unreal Engine 5.8
- C++ / UMG
- World Partition external actors
- CSV-backed simulation data

## Project Structure

```text
Source/Building/       C++ gameplay, cameras, UI, data subsystem, and manager
Content/Untitled/      Main World Partition level and external actor references
Content/Digital_Twins/ Building model, materials, textures, lighting assets
Content/Data/          BMS hourly CSV data and room/building metadata
Content/DigitalTwin/   Runtime temperature-overlay material
Config/                Default game and packaging configuration
Scripts/               Reproducible editor migration/configuration utilities
```

## Run in Unreal Engine

1. Install Unreal Engine 5.8 and Visual Studio with the C++ game-development workload.
2. Clone this repository.
3. Right-click `Building.uproject` and select **Generate Visual Studio project files** if required.
4. Open `Building.sln`, build `BuildingEditor Win64 Development`, then open `Building.uproject`.
5. Run the `/Game/Untitled` level with PIE or Standalone Game.

The project configuration sets the native `DTDigitalTwinGameMode` and `/Game/Untitled` as the default experience.

## Data

`Content/Data/BMSHourly/` contains one CSV file per room and a building-overview dataset. Values are generated for demonstration and visualization purposes; they are not live building data.

## Key C++ Classes

| Class | Responsibility |
| --- | --- |
| `ADigitalTwinManager` | View mode, room selection, simulation time, day/night logic, room overlays |
| `ADTDigitalTwinPlayerController` | Scene/UI input routing and spatial room selection |
| `ADTOrbitCamera` | Automated and manual orbit camera control |
| `ADTRoomCamera` | Shared native room/overview camera type |
| `UDigitalTwinDataSubsystem` | CSV loading and cached BMS/metadata queries |
| `UDigitalTwinControlWidget` | Dashboard UI construction and event binding |

## Notes

This repository is intended as a portfolio demonstration. Packaged executables, Unreal Engine caches, IDE caches, and local build artifacts are intentionally excluded. Ensure that you have permission to publish any third-party model, material, or texture assets before making the repository public.
