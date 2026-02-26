# MSFR - DirectX11-Based 2D Game Framework Portfolio

## 1. Project Overview

MSFR is a personal game framework/prototype built with **C++17, SDL2, and DirectX11**.  
Instead of stopping at basic window rendering, this project focuses on implementing a practical game-development workflow end to end.

Implemented areas include:

- SDL2 window/event handling integrated with a DirectX11 render loop
- State-driven game flow (Splash → MainMenu → GamePlay)
- Action-based input interpretation system (`ActionSystem`)
- Game object/component architecture and player state machine (Idle/Move)
- Texture/sprite/animation loading and rendering
- Engine core systems (logger, job system, texture manager, state manager)

The primary goal was to design a structure that stays easy to extend as features grow.

---

## 2. Tech Stack

- Language: **C++17**
- Graphics API: **DirectX 11**
- Window / Event: **SDL2**
- Build Toolchain: **Visual Studio 2022 (v143 Toolset)**
- Platform: **Windows (x64)**

---

## 3. Runtime Flow

When the game starts, the flow is:

1. **Splash State**
   - Displays a splash image for 5 seconds
   - Can be skipped immediately via Enter/Space/Mouse click
2. **MainMenu State**
   - Displays the menu image
   - Detects Play/HowToPlay/Quit zones by mouse click position
   - Transitions to gameplay on Play click
3. **GamePlay1 State**
   - Renders map texture
   - Spawns player object and handles movement/animation via arrow keys

---

## 4. Architecture

### 4-1. Entry Point and Application Layer

- `main.cpp` creates a `DX11App` instance and runs `Update()` until `IsDone()`.
- `DX11App` handles SDL window creation, D3D11 device/swapchain/RTV/DSV initialization, event pumping, and frame presentation.
- Game-specific logic is abstracted through the `IProgram` interface, currently implemented as `GameProgram`.

This separation keeps **platform/render loop concerns (`DX11App`)** independent from **gameplay logic (`GameProgram` + `Engine`)**.

### 4-2. Engine Singleton Core

`Engine` works as a global service hub:

- `Logger`
- `Input`
- `GameStateManager`
- `TextureManager`
- `JobSystem`
- `ActionSystem`
- `CommandPool`

It also stores DX11 device/context/swapchain pointers for centralized render-system access.

### 4-3. State-Driven Game Flow

`GameStateManager` runs an internal lifecycle state machine:

`START → LOAD → UPDATE → UNLOAD → SHUTDOWN → EXIT`

Core responsibilities:

- Register states: `AddGameState(...)`
- Request transition: `SetNextState(index)`
- Control state lifecycle: `Load() → Update() + Draw() → Unload()`

This makes screen units (Splash/MainMenu/GamePlay) clearly separated and easier to maintain.

### 4-4. Input and ActionSystem

Raw input is collected in `Input`, then translated to game actions in `ActionSystem`.

- Arrow keys (Up/Down/Left/Right) → movement actions
- Enter/Space/Mouse release → `Skip` action

Benefits:

- Gameplay code depends on abstract actions rather than physical key codes
- Lower refactor cost when changing key bindings
- Easier debugging/tracing at action level

### 4-5. Object/Component Structure

- `GameObject` handles position/velocity/state updates
- `Sprite` component handles sprite rendering and animation playback
- `GameObjectManager` centralizes object update/render/collision checks

The player is implemented as `Player` with an internal state machine:

- `StateIdle`: no movement + idle animation
- `StateMove`: movement vector calculation + directional animation

This pattern is intentionally extensible for future states such as Dash/Attack/Hurt.

---

## 5. Key Implementation Highlights

### 5-1. Stable Rendering Pipeline

- D3D11 device/context creation
- Swapchain + backbuffer RTV/depth-stencil creation
- Safe render target recreation after window resize (`ResizeBuffers`)
- Per-frame `Clear → Draw → Present` sequence

### 5-2. State-Based Resource Lifecycle

During state transitions, textures are unloaded and each state’s `Unload()` is called, reducing stale resources and improving memory hygiene.

### 5-3. Input-Driven Menu Interaction

Main menu click handling is implemented via rectangle hit tests based on mouse coordinates, enabling direct interaction without a separate UI framework.

### 5-4. Player Movement and Animation

Movement vectors are computed from directional input, with idle animations selected based on last movement direction. Out-of-bounds fallback resets the player to a start position for stable runtime behavior.

---

## 6. Directory Structure

```text
MSFR/
  Game/
    GameProgram.h
    Splash.*
    MainMenu.*
    GamePlay1.*
    Player.*
  assets/
    images/
    shaders/
  external/
    include/SDL2/
  DX11App.*
  Engine.*
  GameStateManager.*
  GameObject.*
  GameObjectManager.*
  TextureDX11.*
  Sprite.*
  Input.*
  ActionSystem.*
  main.cpp
  MSFR.sln
  MSFR.vcxproj
```

---

## 7. Build and Run

### 7-1. Prerequisites

- Windows 10/11
- Visual Studio 2022 (Desktop development with C++)
- DirectX 11 runtime
- SDL2 headers/libs/`SDL2.dll` in the repository `external` path

### 7-2. Build

1. Open `MSFR/MSFR.sln` in Visual Studio
2. Select `x64 + Debug` or `x64 + Release`
3. Build the solution

The project is configured to copy `external/bin/SDL2.dll` to the target output folder after build.

### 7-3. Run

- Set `MSFR` as the startup project in Visual Studio
- Run the project to launch the game window with runtime logs

---

## 8. Problems Solved and Lessons Learned

### 1) SDL Window + DX11 Initialization Integration

The project required reliable HWND extraction from SDL for DXGI swapchain setup. Using `SDL_SysWMinfo` and enforcing a clear initialization order improved runtime stability.

### 2) Resource Cleanup During State Transitions

To prevent leaks across repeated transitions, unload timing for state resources and the texture manager was explicitly separated.

### 3) Decoupling Input Events from Game Actions

Direct keycode dependency in gameplay logic was reduced by separating raw input (`Input`) from semantic actions (`ActionSystem`).

### 4) Consistent Animation-State Transitions

Inconsistent idle/move animation transitions were improved by explicitly modeling player behavior through a state machine.

---

## 9. Roadmap

- Implement functional HowToPlay / Quit flows
- Add camera system (world-space vs screen-space separation)
- Improve collision handling (AABB + layer/mask)
- Build basic UI framework (button components, text rendering)
- Add audio system (BGM/SFX loading and channel management)
- Move toward data-driven design (JSON-based state/object loading)
- Strengthen editor/debug overlay tooling

---

## 10. Portfolio Strengths Emphasized

- Engine-oriented architecture design separating rendering/input/state/object layers
- C++ system-level implementation with lifecycle and dependency awareness
- Complete playable loop from startup state to interactive gameplay
- Extension-first design mindset rather than one-off feature implementation

---
