# AAA Traffic System – Architecture & Flow (Current State)

## Goals
- Spline‑driven traffic that is road‑tool agnostic (CityBLD or any user spline/mesh).
- Calibration-first: user aligns chevrons on their road, sets lane counts, then builds traffic.
- No bundled road meshes; user visuals are preserved.
- Chaos vehicle visuals via adapter; auto-detect City Sample if present.
- Separate dev tests (automation + PIE) that never ship synthetic roads to users.

## Key Modules
- **TrafficRuntime**
  - `TrafficRoadMetadataComponent`: tags user spline actors with family + include flags.
  - `TrafficRoadFamilySettings`: lane layouts per family (forward/backward counts/widths).
  - `TrafficGeometryProvider` (+ CityBLD provider): collects spline roads + metadata.
  - `TrafficNetworkBuilder`: lanes/intersections/movements from collected roads.
  - `TrafficSystemController`: orchestrates build, holds transient `TrafficNetworkAsset`.
  - `TrafficVehicleBase`: kinematic follower + placeholder cube.
  - `TrafficVehicleAdapter`: attaches any Actor/BP visual (Chaos), hides cube.
  - `TrafficVehicleManager`: spawns vehicles on lanes; adapter hookup + City Sample auto-pick.
  - `TrafficVisualSettings`: ribbon/arrow materials and dimensions (overlays only).
  - `TrafficVehicleSettings`: default test class, adapter toggle, external visual class, City Sample auto-resolve.
- **TrafficEditor**
  - `TrafficSystemEditorSubsystem`: editor hooks (prepare/build/cars/reset), metadata tagging, calibration overlay, no synthetic road generation for users.
  - `LaneCalibrationOverlayActor`: chevron overlay for the selected spline actor (lane widths, directions).
  - `STrafficSystemPanel`: streamlined buttons (Convert Selected, Calibrate, Reset AAA, Reset AAA+Tagged, Build+Cars).
- **TrafficTests (dev-only)**
  - Integration (editor) and PIE tests spawn temporary tagged splines to validate build/cars and adapter visuals. They log network summaries, vehicle positions, PASS markers. Users never see these splines.

## High-Level Flow (User)
1) **Prepare a road actor**: User places/owns spline road meshes (any toolkit). Add/ensure `TrafficRoadMetadataComponent` (Convert Selected does this and tags it).
2) **Calibrate**: Select road actor → Calibrate. Overlay chevrons appear on the user’s road; lane counts & offsets come from family settings or are adjusted. Calibration persists on the actor (metadata family).
3) **Build + Cars**: Runs Prepare → Build → Cars. The controller builds the network from all included roads; vehicle manager spawns traffic along lanes, using adapter visuals if enabled.
4) **Vehicles**: Adapter auto-picks City Sample BP if found; otherwise uses user-set class or placeholder Actor. Visuals attach; logic uses our kinematic follower.
5) **Reset options**: Reset AAA actors (keeps user roads) or Reset AAA + tagged user roads (destructive to tagged roads).

## What’s intentionally removed/disabled
- No user-facing synthetic road generation (legacy RoadLab layout removed).
- Create RoadLab Map button removed from panel.
- Tests use synthetic tagged splines only in automation/PIE runs for developers.

## Logging & Parity
- Build/PIE tests log: network summary (roads/lanes/intersections/movements), vehicle counts, first vehicle locations, adapter visual attachment, PASS markers.
- Road ribbon material logging only affects overlays; user road meshes untouched.

## Current Gaps / Next Work
- Calibration UX improvements: road selector dropdown, lane counts per side, manual offsets, gating Build until calibrated roads exist.
- Intersection logic: richer junction detection/movements for arbitrary road graphs.
- PIE visuals: ensure calibration overlays always sit on user meshes; no synthetic geometry in user flow.
- City Sample path list: add project-specific BP paths (e.g., `/Game/CitySampleVehicles/Blueprint/BP_Vehicle.BP_Vehicle_C`).

## Test Pipelines (dev-only)
- **Automation**: `Project.Traffic.Integration.RoadLab.LayoutBuildCars` — uses temporary tagged splines; logs network + vehicles.
- **PIE**: `Project.Traffic.PIE.RoadLab.BuildCarsAdapter` — same temp splines; validates adapter visual attachment; logs network/vehicles/PASS.

## Default Behaviors
- If adapter is enabled and no external class is set:
  - Try City Sample paths; if none found, fall back to placeholder Actor (still attached so visuals exist).
- If no user roads are in the scene (user flow): Build/Calibrate should no-op with clear warnings (no synthetic roads spawned).
