# MSFR Design Mapping for Required Conditions

This document explains how MSFR is designed to satisfy the following requirements:

1. Error handling and logging
2. Event / messaging system
3. Memory management
4. Command pattern / action system
5. Multithreading with synchronization
6. Isolation of platform-dependent code
7. Automation for testing and demo
8. Game mechanic prototype

---

## 1) Error Handling and Logging

### Design intent
- Fail fast on fatal initialization/runtime errors.
- Keep non-fatal subsystems alive with degraded behavior (for example audio disabled).
- Record structured, timestamped logs for troubleshooting and balance analysis.

### Core structure
- `Logger` provides severity levels (`Verbose`, `Debug`, `Event`, `Warning`, `Error`, `Fatal`) and contextual logging (`subsystem`, `file`, `function`, `line`, optional code).
- `ENGINE_LOG_CTX` / `ENGINE_LOG_HRESULT` macros attach source context automatically.
- Main app wraps the full runtime in `try/catch` and reports fatal errors via both log and message box.

### Runtime flow
- Startup failures throw exceptions in platform/render init paths and are surfaced to `main()` catch.
- Recoverable failures log warnings and continue with fallback behavior.
- Logs are flushed to `Trace.log` and debug output in real time.

### Code evidence
- Logger severity/context and mutex-protected sink: `Engine/Logger.h:10`, `Engine/Logger.h:19`, `Engine/Logger.h:57`, `Engine/Logger.cpp:76`, `Engine/Logger.cpp:108`
- File + debug output + flush: `Engine/Logger.cpp:33`, `Engine/Logger.cpp:121`, `Engine/Logger.cpp:125`
- Fatal logging and throw on SDL init/window failures: `Engine/DX11App.cpp:413`, `Engine/DX11App.cpp:415`, `Engine/DX11App.cpp:431`
- HRESULT-based throw helper in renderer backend: `Engine/DX11RenderBackend.cpp:30`, `Engine/DX11RenderBackend.cpp:38`
- Global exception boundary in main: `main.cpp:65`, `main.cpp:85`, `main.cpp:87`
- Graceful degradation example (audio warnings without crash): `Engine/Engine.cpp:75`, `Engine/Engine.cpp:77`, `Engine/AudioSystem.cpp:42`, `Engine/AudioSystem.cpp:96`

---

## 2) Event / Messaging System

### Design intent
- Decouple gameplay/UI trigger points from transition and side-effect execution.
- Make publish sites simple and subscriber logic centralized.
- Avoid mutation during event iteration by queueing events.

### Core structure
- `EventBus` supports typed subscriptions, unsubscription, queued publish, and batched dispatch.
- Events are stored as `type_index + shared_ptr<const void>` and dispatched to copied listener lists.
- `Engine` exposes singleton bus; game states publish, `GameProgram` subscribes.

### Runtime flow
- Producer state publishes event (`RequestStateChangeEvent`, `MenuActionEvent`).
- `Engine::Update()` dispatches queued events before and during fixed-step simulation.
- Subscriber callback executes pooled command (`RequestStateChangeCommand`, `LogMenuActionCommand`).

### Code evidence
- Typed subscribe/publish/dispatch and thread-safe locks: `Engine/EventBus.h:20`, `Engine/EventBus.h:51`, `Engine/EventBus.h:61`, `Engine/EventBus.h:116`, `Engine/EventBus.h:119`
- Event definitions: `Engine/EventTypes.h:10`, `Engine/EventTypes.h:15`
- Engine dispatch points in frame loop: `Engine/Engine.cpp:164`, `Engine/Engine.cpp:172`
- Publish sites in gameplay/menu: `Game/MainMenu.cpp:44`, `Game/MainMenu.cpp:50`, `Game/GamePlay1.cpp:215`, `Game/GamePlay1.cpp:233`, `Game/Splash.cpp:33`
- Subscription and command execution bridge: `Engine/GameProgram.h:330`, `Engine/GameProgram.h:331`, `Engine/GameProgram.h:335`

---

## 3) Memory Management

### Design intent
- Use RAII as default ownership model.
- Reduce runtime allocation churn for hot-path objects/commands using pools.
- Make teardown deterministic for state transitions and engine shutdown.

### Core structure
- `std::unique_ptr` ownership in managers (`GameObjectManager`, `ComponentManager`, `Engine` internals).
- Fixed-capacity `CommandPool<2048,64>` with placement-new and explicit destroy.
- Generic `ObjectPool<T, Capacity, LockPolicy>` utility for fixed-slot object lifetime control.
- Bullet runtime uses reusable `BulletPool` vectors with active/free index lists and overflow recycling.

### Runtime flow
- State loads create owned objects/components via `unique_ptr` and register with manager.
- Update/reclaim cycle resets destroyed entries and reuses freed indices.
- On unload/shutdown, RAII destructors and explicit clear/reset paths release resources.

### Code evidence
- Engine-owned subsystems and pools: `Engine/Engine.h:168`, `Engine/Engine.h:176`, `Engine/Engine.h:177`
- Component unique ownership: `Engine/ComponentManager.h:35`, `Engine/ComponentManager.h:66`
- Game object ownership and pending-add queue: `Engine/GameObjectManager.h:42`, `Engine/GameObjectManager.cpp:15`, `Engine/GameObjectManager.cpp:49`, `Engine/GameObjectManager.cpp:61`
- Command pool slot allocator: `Engine/CommandPool.h:11`, `Engine/CommandPool.h:29`, `Engine/CommandPool.h:56`
- Generic object pool utility: `Engine/Objectpool.h:21`, `Engine/Objectpool.h:52`, `Engine/Objectpool.h:73`
- Bullet pool reuse model: `Game/BulletPool.h:55`, `Game/BulletPool.cpp:96`, `Game/BulletPool.cpp:103`, `Game/BulletPool.cpp:131`

---

## 4) Command Pattern / Action System

### Design intent
- Separate "input intent" from "gameplay action" and from "state transition execution".
- Use command objects for explicit, testable transition side effects.

### Core structure
- Command pattern:
  - `ICommand` interface with `Execute()`.
  - Concrete commands in `GameCommands.h`.
  - Pooled command creation/execution/destroy helper in `GameProgram`.
- Action system:
  - `ActionId` enum as game-level intent vocabulary.
  - `ActionSystem::PollFromInput()` maps raw input state to fired actions.
  - Gameplay/splash/player query `ActionSystem::Has(...)`.

### Runtime flow
- Per-frame: `DX11App::Update()` polls input -> updates action system.
- States consume high-level actions (`Skip`, movement actions).
- State-change events trigger command execution via event subscription.

### Code evidence
- Command abstraction: `Engine/Command.h:3`
- Concrete commands: `Engine/GameCommands.h:9`, `Engine/GameCommands.h:24`
- Pooled execute helper: `Engine/GameProgram.h:17`, `Engine/GameProgram.h:21`, `Engine/GameProgram.h:29`
- Action ids and poller: `Engine/ActionId.h:4`, `Engine/ActionSystem.cpp:4`, `Engine/ActionSystem.cpp:36`
- Frame integration: `Engine/DX11App.cpp:618`
- Action consumers: `Game/Splash.cpp:30`, `Game/Player.cpp:206`

---

## 5) Multithreading with Synchronization

### Design intent
- Parallelize high-count object updates while keeping deterministic frame boundaries.
- Protect shared queues/state with clear lock ownership.
- Expose thread profiler stats for runtime verification.

### Core structure
- `JobSystem` worker pool with queue + condition variables:
  - `queueMutex + cvWork` for work handoff.
  - `idleMutex + cvIdle` for completion waits.
  - atomics for lifecycle and pending-job tracking.
- `GameObjectManager` dispatches snapshot updates in parallel and waits idle before post-step reclamation.
- `EventBus` and `Logger` are both lock-protected for concurrent access safety.

### Runtime flow
- On engine init, workers are created (hardware concurrency minus main thread when possible).
- Manager builds object snapshot, dispatches chunked jobs, waits idle, then performs reclaim/flush.
- Profiler overlay reads worker snapshots and pending jobs.

### Code evidence
- JobSystem synchronization primitives: `Engine/JobSystem.h:105`, `Engine/JobSystem.h:106`, `Engine/JobSystem.h:109`, `Engine/JobSystem.h:112`, `Engine/JobSystem.h:113`
- Worker loop wait/pop/run: `Engine/JobSystem.cpp:148`, `Engine/JobSystem.cpp:149`, `Engine/JobSystem.cpp:156`, `Engine/JobSystem.cpp:172`
- Idle wait and completion signal: `Engine/JobSystem.cpp:95`, `Engine/JobSystem.cpp:193`, `Engine/JobSystem.cpp:197`
- Parallel object update: `Engine/GameObjectmanager.cpp:97`, `Engine/GameObjectmanager.cpp:100`, `Engine/GameObjectmanager.cpp:108`
- Thread-safe logging and events: `Engine/Logger.cpp:117`, `Engine/EventBus.h:30`, `Engine/EventBus.h:57`
- Runtime profiler view of worker stats: `Engine/DX11App.cpp:243`, `Engine/DX11App.cpp:268`, `Engine/DX11App.cpp:274`

---

## 6) Isolation of Platform-Dependent Code

### Design intent
- Keep rendering and native window/device specifics behind backend/app boundaries.
- Leave gameplay/state logic independent from D3D/Win32 APIs.

### Current isolation pattern
- Rendering abstraction via `IRenderBackend` interface.
- DX11 implementation isolated in `DX11RenderBackend`.
- App bootstrap/event pump in `DX11App` mediates SDL/Win32/native handle handoff.
- Gameplay code (`Game/*`) interacts with engine services, not DirectX APIs.
- Win32-only input message handling is isolated in `Engine/InputWin32.cpp`.
- Win32-only logger bridge (`OutputDebugStringA`, focus restore) is isolated in `Engine/PlatformLoggerWin32.cpp`.

### Code evidence
- Backend interface boundary: `Engine/RenderBackend.h:3`, `Engine/RenderBackend.h:8`
- DX11 concrete backend: `Engine/DX11RenderBackend.h:11`, `Engine/DX11RenderBackend.cpp:50`
- App-level backend ownership and use: `Engine/DX11App.h:68`, `Engine/DX11App.cpp:108`, `Engine/DX11App.cpp:621`, `Engine/DX11App.cpp:642`
- Engine exposes device pointers without direct gameplay coupling: `Engine/Engine.h:129`, `Engine/Engine.cpp:28`
- Logger core now depends on platform hooks rather than Win32 headers: `Engine/Logger.cpp:38`, `Engine/Logger.cpp:123`, `Engine/Logger.cpp:132`, `Engine/PlatformLogger.h:5`, `Engine/PlatformLoggerWin32.cpp:10`
- Input core is platform-neutral and Win32 message translation is isolated: `Engine/input.cpp:1`, `Engine/InputWin32.cpp:50`

---

## 7) Automation for Testing and Demo

### Design intent
- Provide one-command build + smoke verification locally.
- Mirror the same verification in CI for every push/PR.

### Core structure
- `scripts/build-and-demo.ps1`: discovers MSBuild, builds solution, then invokes smoke run.
- `scripts/demo-smoke.ps1`: launches executable with auto-exit, validates `Trace.log` marker, and restores prior log state.
- GitHub Actions workflow runs Debug/Release matrix build + smoke + artifact upload.

### Runtime flow
- Local:
  - Build selected config.
  - Run `MSFR.exe --auto-exit-ms=...`.
  - Verify `Trace.log` exists and contains `Engine InitCore`.
- CI:
  - Same smoke script in Windows runner for each matrix config.

### Code evidence
- Build automation script: `scripts/build-and-demo.ps1:19`, `scripts/build-and-demo.ps1:26`, `scripts/build-and-demo.ps1:32`
- Smoke assertions: `scripts/demo-smoke.ps1:34`, `scripts/demo-smoke.ps1:51`, `scripts/demo-smoke.ps1:56`
- CI workflow: `.github/workflows/c-cpp.yml:14`, `.github/workflows/c-cpp.yml:25`, `.github/workflows/c-cpp.yml:30`
- Runtime auto-exit/auto-play args: `main.cpp:32`, `main.cpp:67`, `main.cpp:68`, `README.md:221`

---

## 8) Game Mechanic Prototype

### Prototype goal
- Deliver a complete playable loop with objective progression, combat, pacing, and end-state transitions.

### Core gameplay loop (`GamePlay1`)
- Objective: collect all `DataCore` objects while surviving enemy pressure.
- Combat: switchable weapon modes (machine gun / shotgun), pooled projectiles, enemy hit/kill accounting.
- Difficulty pacing: phase-based multipliers (early/mid/late) affecting spawn interval, max enemies, and fire cooldown.
- End states:
  - Player death -> publish GameOver transition.
  - All cores collected -> publish Credit transition.
- Telemetry: periodic `[BalanceLog]` ticks + run-end summary stored in engine state and shown in end screens.

### Code evidence
- State setup + world/objective bootstrap: `Game/GamePlay1.cpp:41`, `Game/GamePlay1.cpp:71`, `Game/GamePlay1.cpp:123`
- Bullet pool setup and updates: `Game/GamePlay1.cpp:135`, `Game/GamePlay1.cpp:188`, `Game/GamePlay1.cpp:205`
- Weapon input + firing logic: `Game/GamePlay1_Combat.cpp:59`, `Game/GamePlay1_Combat.cpp:96`, `Game/GamePlay1_Combat.cpp:115`
- Enemy spawn/AI/contact damage: `Game/GamePlay1_Enemy.cpp:317`, `Game/GamePlay1_Enemy.cpp:380`, `Game/GamePlay1_Enemy.cpp:490`
- Bullet-enemy hit resolution and kill counting: `Game/GamePlay1_Enemy.cpp:518`, `Game/GamePlay1_Enemy.cpp:577`, `Game/GamePlay1_Enemy.cpp:583`
- Objective and transitions: `Game/GamePlay1.cpp:221`, `Game/GamePlay1.cpp:231`, `Game/GamePlay1.cpp:215`
- Phase balancing and logs: `Game/GamePlay1.cpp:437`, `Game/GamePlay1.cpp:460`, `Game/GamePlay1.cpp:478`
- Data-driven tuning file reload and validation: `Game/BalanceConfig.cpp:361`, `Game/BalanceConfig.cpp:407`, `Game/BalanceConfig.h:68`

---

## Requirement Coverage Summary

- 1. Error handling & logging: Implemented (fatal + recoverable paths, structured logs).
- 2. Event/messaging system: Implemented (`EventBus` with typed subscriptions + queued dispatch).
- 3. Memory management: Implemented (RAII + pool allocators + reuse paths).
- 4. Command/action system: Implemented (command pattern + action mapping layer).
- 5. Multithreading + synchronization: Implemented (`JobSystem`, mutex/CV/atomics, parallel object update).
- 6. Platform-dependent isolation: Implemented (backend boundary + Win32 bridges for input/logger).
- 7. Automation for test/demo: Implemented (PowerShell scripts + GitHub Actions smoke matrix).
- 8. Game mechanic prototype: Implemented (playable objective/combat/pacing/end-state loop).

