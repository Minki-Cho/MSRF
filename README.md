# MSFR - Multithreaded Simulation Rendering Framework
<img width="1536" height="1024" alt="Splash" src="https://github.com/user-attachments/assets/0705bf37-f5fa-4c27-809d-92fb945d9078" />

## 1. Project Overview

MSFR is a personal 2D game framework/prototype built with **C++17 + SDL2 + DirectX11**.
The project focuses on structuring a real gameplay pipeline, not just rendering a window.

Current implementation includes:

- SDL2 event loop integrated with a DirectX11 renderer
- State-based flow: `Splash -> MainMenu -> GamePlay1`
- `Input` to `ActionSystem` translation layer
- `GameObject` + component architecture with a player FSM (`Idle`, `Move`)
- Sprite/animation loading from `.spt` / `.anm`
- Engine-level services: logger, job system, texture manager, state manager, event bus, command pool
- ImGui-based runtime profiler overlay

---

## 2. Recent Updates

- Added **ImGui profiler overlay** (`F2` toggle): frame time/FPS graph, worker count, pending jobs, command-pool usage
- Added **EventBus + Command pattern** for state change and menu-action processing
- Added **fixed-size CommandPool** (`CommandPool<2048, 64>`) for command allocation
- Upgraded object update path with **JobSystem-based parallel dispatch**
- Added **camera follow + clamp** behavior in `GamePlay1`
- Added **collision debug draw toggle** (`~`)
- Added **automation scripts and CI smoke run**:
  - `scripts/build-and-demo.ps1`
  - `scripts/demo-smoke.ps1`
  - GitHub Actions workflow (`.github/workflows/c-cpp.yml`)
- Replaced blocking SDL message-box menus with in-game ImGui overlays for `MainMenu`, `HowToPlay`, `GameOver`, and gameplay pause
- Added run-result summary on `Credit`/`GameOver` (survival time, enemies defeated, core collection rate)
- Applied first-pass gameplay rebalance in `assets/config/gameplay_balance.cfg` (spawn pacing, enemy stats, weapon cadence)

---

## 3. Tech Stack

- Language: **C++17**
- Graphics API: **DirectX 11**
- Window/Event: **SDL2**
- UI Overlay: **Dear ImGui** (`imgui_impl_sdl2`, `imgui_impl_dx11`)
- Build Toolchain: **Visual Studio 2022 (v143)**
- Platform: **Windows x64**

---

## 4. Runtime Flow

1. **Splash**
- Displays `assets/images/Splash.png`
- Automatically transitions after about 5 seconds
- Can be skipped immediately via `Enter`, `Space`, or mouse left-button release

2. **MainMenu**
- Displays `assets/images/MainMenu.png`
- Supports `Play / HowToPlay / Quit` via mouse click
- Keyboard shortcuts:
  - `Enter` or `Space`: Play
  - `H`: HowToPlay
  - `Esc`: Quit confirmation
- `HowToPlay` opens a dedicated overlay state and lets you choose `Start Game` or `Back`
- `Quit` uses a non-blocking in-game confirmation overlay

3. **GamePlay1**
- Creates `GameObjectManager` and spawns `Player`
- Renders map texture (`assets/images/map.png`)
- Updates player movement and animation
- Applies player-follow camera with world-boundary clamping

---

## 5. Controls

- `Arrow Keys`: player move action
- `Enter`, `Space`, `Mouse Left Release`: splash skip action
- `Esc` (in gameplay): pause menu (`Resume / Restart / Main Menu / Quit`)
- `F1`: logger console on/off
- `F2`: profiler overlay on/off
- `` ` `` (tilde): collision debug draw on/off

---

## 6. Architecture

### 6-1. App Layer (`DX11App`)

- Creates SDL window, extracts HWND, and initializes D3D11 device/context/swapchain
- Creates back-buffer RTV/DSV and recreates them on resize
- Frame loop: input update -> event pump -> action poll -> game update/draw -> ImGui draw -> present

### 6-2. Engine Core (`Engine`)

Global service hub:

- `Logger`
- `Input`
- `GameStateManager`
- `TextureManager`
- `JobSystem`
- `ActionSystem`
- `CommandPool`
- `EventBus`

Also stores DX11 pointers and per-frame timing values (`dt`, `ms`, `fps`).

### 6-3. State System (`GameStateManager`)

Lifecycle state machine:

`START -> LOAD -> UPDATE -> UNLOAD -> SHUTDOWN -> EXIT`

Responsibilities:

- state registration: `AddGameState(...)`
- transition request: `SetNextState(index)`
- lifecycle execution: `Load() -> Update()/Draw() -> Unload()`

### 6-4. Event + Command Flow

- Game states publish events (`RequestStateChangeEvent`, `MenuActionEvent`)
- `GameProgram` subscribes and executes commands via pool:
  - `RequestStateChangeCommand`
  - `LogMenuActionCommand`

This decouples gameplay triggers from transition execution details.

### 6-5. Object/Component Layer

- `GameObject` owns transform/state and per-object components
- `Sprite` loads texture/animations/collision data from asset descriptors
- `GameObjectManager` updates objects in parallel through `JobSystem`
- Optional collision debug rendering is drawn per object component

---

## 7. Directory Structure

```text
MSFR/
  Engine/
    DX11App.*
    Engine.*
    GameStateManager.*
    EventBus.h
    GameCommands.h
    CommandPool.h
    JobSystem.*
    TextureDX11.*
    Sprite.*
    GameObject.*
    GameObjectManager.*
    Input.*
    ActionSystem.*
  Game/
    Splash.*
    MainMenu.*
    GamePlay1.*
    Player.*
    ScreenMods.h
  assets/
    images/
    shaders/
  external/
    include/SDL2/
    imgui/
  main.cpp
  MSFR.sln
  MSFR.vcxproj
scripts/
  build-and-demo.ps1
  demo-smoke.ps1
.github/workflows/
  c-cpp.yml
```

---

## 8. Build and Run

### 8-1. Prerequisites

- Windows 10/11
- Visual Studio 2022 (Desktop development with C++)
- DirectX 11 runtime
- SDL2 headers/libs/`SDL2.dll` included in the repository (`MSFR/external`)

### 8-2. Build (Visual Studio)

1. Open `MSFR/MSFR.sln`
2. Select `x64` + `Debug` or `Release`
3. Build

The solution builds `EngineLib` first and then links `MSFR`.
The `MSFR.vcxproj` post-build step copies `external/bin/SDL2.dll` to the output folder.

### 8-3. Run

- Set `MSFR` as the startup project and run

### 8-4. Scripted Build + Smoke

```powershell
powershell -ExecutionPolicy Bypass -File scripts/build-and-demo.ps1 -Configuration Debug -AutoExitMs 7000
```

Smoke run checks:

- executable launch/exit
- whether `Trace.log` is created
- whether the `Engine InitCore` marker exists

### 8-5. Balance Tuning

- Gameplay balance values are loaded from:
  - `MSFR/assets/config/gameplay_balance.cfg`
- You can tune weapon, enemy, spawn, and phase (`early/mid/late`) values without recompiling code.
- The file is reloaded each time `GamePlay1` state enters `Load()`.
- Runtime balance logs are emitted as `[BalanceLog] ...` lines in `Trace.log`.
- For automated gameplay-entry profiling, run:
  - `MSFR.exe --auto-play --auto-exit-ms=65000`

### 8-6. Release Package (GitHub Release)

Create a distributable zip locally:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/package-release.ps1 -Configuration Release -Version v1.0.0
```

Output:

- `artifacts/MSFR-v1.0.0-win64.zip`
- zip contents:
  - `MSFR.exe`
  - `SDL2.dll`
  - `assets/`
  - `README.md`, `LICENSE`

GitHub Release automation:

- Push a tag like `v1.0.0`
- Workflow `.github/workflows/release.yml` builds `Release|x64`, packages zip, and uploads it to GitHub Releases

---

## 9. CI

GitHub Actions (`Windows Build & Demo Smoke`) runs:

- matrix build (`Debug`, `Release`)
- `demo-smoke.ps1` auto-exit run
- trace log artifact upload

---

## 10. Known Gaps / Next Steps

- Collision system is currently focused on debug rendering/basic checks and needs gameplay integration
- A full custom in-game UI framework is still pending (current menus use ImGui overlays)
- Data-driven loading (e.g., JSON) and editor tooling remain on the roadmap
