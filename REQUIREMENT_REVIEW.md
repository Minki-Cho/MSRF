# Project Requirement Review

평가 기준: 코드에 **실제 구현 및 연결(wiring)** 이 되어 있는지 중심으로 판단.

## 1) Error handling & logging — **충족 (Strong)**
- `main()` 에서 전체 런루프를 `try/catch` 로 감싸 치명 오류를 사용자에게 메시지박스로 표시함.
- `Logger` 가 파일(`Trace.log`) + 디버그 출력 + 콘솔 출력을 지원하고, mutex로 동기화되어 멀티스레드 로그 안전성을 확보함.
- DX11/SDL 초기화 및 리사이즈 경로에서 실패 시 예외를 던지는 방어 코드가 다수 존재.

## 2) Some type of event / messaging system — **부분 충족 (Medium)**
- `ActionSystem` 이 입력을 `ActionId` 이벤트로 변환(`PollFromInput`)하여 게임 로직이 `Has(ActionId::Skip)` 로 소비하는 구조가 있음.
- 다만 일반화된 이벤트 버스(pub/sub, 큐, payload 포함)는 없고 현재는 제한된 액션 집합 위주.

## 3) Memory management — **부분~충족 (Medium/Strong)**
- `ObjectPool<T>` / `CommandPool` 구현으로 고정 용량 풀 기반 할당/해제를 제공.
- 그러나 엔진 전반에서 여전히 raw pointer + `new/delete` 사용이 많아(예: `TextureManager`, `GameObjectManager`) RAII/스마트 포인터 일관성은 낮음.

## 4) Command pattern / action system — **부분 충족 (Medium)**
- `ICommand::Execute()` 인터페이스와 `CommandPool` 은 준비되어 있음.
- 하지만 현재 코드에서 실제 커맨드 타입 정의/실행 흐름(`Execute` 호출 체인)은 확인되지 않음.
- 반면 ActionSystem 기반 액션 흐름은 실제 동작 중.

## 5) Multithreading with synchronization — **충족 (Strong)**
- `JobSystem` 에 worker thread, 작업 큐, condition_variable, atomic pending counter, idle wait가 구현됨.
- `GameObjectManager::Update` 에서 snapshot + `Dispatch` + `WaitIdle` 로 병렬 업데이트를 사용.
- pending 추가에는 mutex로 보호됨.

## 6) Platform-dependent code is isolated — **부분 충족 (Medium)**
- Windows/DX11/SDL 세부 구현이 `DX11App`, `Window`, `DX11Services` 등으로 어느 정도 분리됨.
- 그러나 `Engine.h` 가 `Window.h`/D3D 타입을 직접 포함하고, 여러 파일에서 `Windows.h` 의존이 보여 완전한 플랫폼 추상화 계층까지는 아님.

## 7) Automation for testing & demo — **충족 (Baseline)**
- GitHub Actions 워크플로우(`.github/workflows/windows-smoke.yml`)를 추가해 Windows runner에서
  1) `Release|x64` 빌드,
  2) 산출물(`MSFR.exe`) 탐색,
  3) 5초 데모 스모크 실행(정상 실행 확인 후 종료)
  를 자동화함.
- 로컬에서도 같은 흐름을 재현할 수 있도록 `scripts/build_release.ps1`, `scripts/demo_smoke.ps1`를 추가함.

## 8) Game mechanic prototype — **부분 충족 (Medium)**
- 상태 전환(Splash→MainMenu), 입력 클릭 처리, 충돌 테스트 루프 등 프로토타입 성격의 기초 메커닉은 존재.
- 다만 `GamePlay1` 은 아직 플레이 메커닉이 얕고(배경 표시 + 로그 수준), 핵심 게임플레이 규칙이 충분히 구현됐다고 보기 어려움.

---

## 종합 판단 (업데이트 반영)
- **명확히 충족**: 1, 5, 7
- **부분 충족**: 2, 3, 4, 6, 8

즉, 자동화(#7)는 최소 기준으로 충족됐고, 다음으로는 #4/#8을 강화하면 전체 완성도가 크게 올라감.

## 우선 보완 권장 순서
1. **#4 완성**: 실제 `ICommand` 구현체(예: `SkipSplashCommand`, `ChangeStateCommand`)와 실행 경로 추가.
2. **#8 강화**: 승패/점수/적 AI/반복 가능한 core loop 중 1~2개를 명시적으로 구현.
3. **#3 정리**: 소유권을 `unique_ptr` 중심으로 바꾸고 raw `new/delete` 축소.
4. **#2 확장**: 범용 이벤트 버스(타입, payload, subscriber) 도입.
5. **#6 개선**: 플랫폼 의존 코드 헤더 분리/추상화 계층 추가.
