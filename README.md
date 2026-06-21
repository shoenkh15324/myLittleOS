# myLittleOS

> A Layered Application Framework written in C — OS-inspired architecture with Active Object concurrency model.

[![Language](https://img.shields.io/badge/language-C-blue)]()
[![C++](https://img.shields.io/badge/C++-20-%2300599C)]()
[![CMake](https://img.shields.io/badge/CMake-3.10+-064F8C)]()
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Win32-lightgrey)]()

---

## Table of Contents

1. [Overview](#overview)
2. [Features](#features)
3. [Architecture](#architecture)
4. [Core Components](#core-components)
5. [Project Structure](#project-structure)
6. [Getting Started](#getting-started)
7. [Example Applications](#example-applications)
8. [Development Status](#development-status)

---

## Overview

이 프로젝트는 **OS 커널의 계층적 아키텍처에서 영감을 받아 설계된 애플리케이션 프레임워크**입니다.

단순한 라이브러리나 엔진이 아닌, **Core(Kernel) → Driver → Service → App**의 4계층 구조와 **Active Object(액터) 패턴 기반의 동시성 모델**, 그리고 **OSAL(OS Abstraction Layer)** 을 핵심으로 합니다.

그래픽 렌더링 기능은 이 아키텍처의 **확장성을 검증하기 위한 하나의 예시**일 뿐이며, 프로젝트의 진정한 가치는 다음과 같습니다.

- 계층 간 명확한 책임 분리와 의존성 방향
- Compile-time configuration 기반의 유연한 모듈 구성
- 크로스 플랫폼 추상화 (Linux / Win32, Embedded placeholder)
- Active Object 패턴을 통한 thread-safe 동시성 모델

---

## Features

### Architectural

- **4-Layer Architecture**: Core(Kernel) → Driver → Service → App. 각 계층은 명확한 인터페이스로 분리되어 독립적인 개발과 테스트가 가능합니다.
- **Compile-Time Configuration**: `appConfig.h` 하나로 MCU, OS, SDK, Board부터 Driver, Service, Memory 모델, Log 수준까지 모든 것을 define 기반으로 선택합니다.
- **OS Abstraction Layer**: Thread, Mutex, Semaphore, Timer, Epoll, Memory를 Linux(pthreads)와 Win32(Win32 API)로 추상화하며, Embedded(FreeRTOS/Zephyr)를 위한 placeholder도 정의되어 있습니다.

### Concurrency

- **Active Object Pattern**: 각 액터는 전용 스레드, 이벤트 루프, 주기적 타이머, Async 메일박스를 가집니다. 객체 상태(Closed → Opening → Opened → Closing) 기반의 생명주기를 관리합니다.
- **Async Messaging**: Publish/subscribe 기반으로 eventId 범위에 따라 target 액터로 자동 라우팅됩니다. 4가지 전송 타입(Async / AsyncPayload / Await / Express)을 지원합니다.
- **Lock-Free Ring Buffer**: Single-Producer Single-Consumer 패턴, memory barrier 기반으로 동기화 없이 동작합니다. Push overwrite 옵션과 통계 기능을 제공합니다.

### Extensibility

- Interface 기반의 Driver 등록 시스템 (`driverDefs.h` → `driverCommon.c` dispatch)
- Interface 기반의 Service 등록 시스템 (`serviceDefs.h` → `serviceCommon.c` dispatch)
- Platform에 독립적인 App 작성 가능 (appConfig.h만 교체하면 OS/Driver 변경)

---

## Architecture

### Layer Diagram

```
┌──────────────────────────────────────────────────────────────┐
│                       Application                             │
│   (sample / engine_2d / blackhole_simulation)                 │
│                                                               │
│   activeObject[]  │  appEvent handlers  │  appConfig.h        │
├──────────────────────────────────────────────────────────────┤
│                       Service                                 │
│   Rendering2D / Rendering3D / ...                             │
│                                                               │
│   renderQueue  │  camera management  │  draw commands         │
├──────────────────────────────────────────────────────────────┤
│                       Driver                                  │
│   Platform(Win32) │ GFX(OpenGL/bgfx) │ Physics(Jolt/custom)   │
│                                                               │
│   window handling │ draw calls │ physics step                 │
├──────────────────────────────────────────────────────────────┤
│                     Core (Kernel)                             │
│   OSAL │ Log │ Async │ Active Object │ Ring Buffer            │
│                                                               │
│   systemOpen → init → idle loop                               │
└──────────────────────────────────────────────────────────────┘
```

### System Initialization Sequence

```
main()
  └── systemOpen()
        ├── printf(system banner)
        ├── logOpen()
        │     └── osalMutexOpen(&_logMutex)
        ├── driverCommonOpen()
        │     ├── driverPlatformWin32Open()  // window class registration
        │     ├── driverJoltOpen()           // Jolt physics system init
        │     └── driverBgfxOpen()           // bgfx renderer + mesh creation
        ├── serviceCommonOpen()
        │     └── serviceRendering3dOpen()   // scene setup, camera init
        ├── appCommonOpen()
        │     └── appOpen()
        │           ├── activeOpen(mainActor) // thread create + subscribe + timer start
        │           └── asyncPush(createWindow, bgfxInit, physicsInit, ...)
        └── while(1)
              └── osalSleepMs(1000)          // main thread idle
```

### Async Event Flow

```
Sender Thread                  Async Router                  Receiver Thread
     │                             │                              │
     │  asyncPush(eventId, args)   │                              │
     ├────────────────────────────▶│                              │
     │                             │  _asyncLookupTarget(eventId) │
     │                             │  → find activeObject by      │
     │                             │    eventId range             │
     │                             │                              │
     │                             │  bufferPush(eventQueue[],    │
     │                             │             asyncPacket)     │
     │                             │                              │
     │                             │  osalEpollNotify()           │
     │                             │  (or osalSemaphoreGive())    │
     │                             ├─────────────────────────────▶│
     │                             │                              │  epoll_wait returned
     │                             │                              │  → eventFd triggered
     │                             │                              │
     │                             │                              │  while(asyncPop())
     │                             │                              │    → appThreadHandler()
     │                             │                              │
```

---

## Core Components

### OSAL — OS Abstraction Layer

OSAL은 서로 다른 OS의 시스템 프리미티브를 **동일한 API로 추상화**합니다. 각 기능은 `#if APP_OS == OS_LINUX / OS_WIN32` 분기로 구현되며, Embedded 타겟을 위한 placeholder도 정의되어 있습니다.

```c
// Usage example — single API, multiple platforms
osalThreadOpen(&thread, &attr, myThreadFunc, arg);
osalMutexLock(&mutex, timeoutMs);
osalSemaphoreGive(&sema);
osalTimerOpen(&timer, callback, arg, periodMs);
```

| Category | Linux | Win32 | Embedded (placeholder) |
|----------|-------|-------|----------------------|
| Thread | `pthread_create` / `pthread_join` | `CreateThread` / `WaitForSingleObject` | config |
| Mutex | `pthread_mutex_*` (recursive) | `CreateMutex` / `ReleaseMutex` | config |
| Semaphore | `sem_*` (POSIX) | `CreateSemaphore` / `ReleaseSemaphore` | config |
| Timer | `timerfd_create` / `timerfd_settime` | `CreateWaitableTimer` / `SetWaitableTimer` | config |
| Epoll | `epoll_create1` / `epoll_wait` + `eventfd` | `MsgWaitForMultipleObjects` + Win32 message | config |
| Memory | `malloc` / `free` | `malloc` / `free` | Static pool / Dynamic |

### Active Object — Actor Model

Active Object 패턴은 이 프레임워크의 **동시성 모델의 핵심**입니다. 각 `activeObject` 인스턴스는 하나의 액터로 동작합니다.

```c
typedef struct activeObject {
    objectState     objState;           // Closed → Opening → Opened → Closing
    osalMutex       objMutex;           // 상태 보호
    osalSemaphore   objSema;            // 이벤트 대기 (Win32)
    osalEpoll       objEpoll;           // 이벤트 대기 (Linux: eventFd + timerFd)
    ringBuffer      eventQueue[];       // Sender별 lock-free 큐
    osalThread      appThread;          // 전용 스레드
    void          (*appThreadHandler)(); // 이벤트 디스패치 콜백
    osalTimer       appTimer;           // 주기적 타이머
    void          (*appTimerHandler)();  // 타이머 만료 콜백
    bool            isMainThread;       // Win32 메시지 펌프 소유 여부
    uint16_t        appEventIdxStart;   // 구독 eventId 시작
    uint16_t        appEventIdxEnd;     // 구독 eventId 끝
} activeObject;
```

**Actor Lifecycle:**
```
objStateClosed → [activeOpen] → objStateOpening → ... → objStateOpened
                                                              │
                                             ┌────────────────┤
                                             ▼                ▼
                                     Linux epoll_wait    Win32 WaitForMultipleObjects
                                     ├─ eventFd: async   ├─ semaphore: async
                                     ├─ timerFd: timer   ├─ timer: timer
                                     └─ ...              └─ msg pump (main thread)
                                                              │
objStateClosed ← [activeClose] ← objStateClosing ← ... ← objStateOpened
```

**Event Loop (Linux example):**
```c
static void _actorThreadHandler(void* arg) {
    activeObject* actor = (activeObject*)arg;
    while(1) {
        osalEpollWait(&actor->objEpoll, &fd, -1);
        if(fd == actor->objEpoll.eventFd) {
            read(fd, &expired, sizeof(expired));
            while(asyncPop(actor, &async, payload))
                actor->appThreadHandler(actor, &async, payload);
        } else if(actor->isMainThread && fd == actor->appTimer.hTimer) {
            read(fd, &expired, sizeof(expired));
            if(actor->appTimerHandler)
                actor->appTimerHandler(actor);
        }
    }
}
```

### Async Messaging

이벤트 ID 기반의 **publish/subscribe** 시스템입니다. `asyncPush()` 호출 시 eventId 범위를 보고 target `activeObject`를 찾아 라우팅합니다.

```c
typedef enum {
    asyncTypeAsync = 0,        // 비동기 메시지 (arg1~arg4)
    asyncTypeAsyncPayload,     // Payload 포함 (ring buffer에 별도 저장)
    asyncTypeAwait,            // 동기 대기 (TODO)
    asyncTypeExpress,          // 고속 전송 (TODO)
} asyncType;

typedef struct asyncPacket {
    asyncType type;
    uint16_t eventId;
    uintptr_t arg1, arg2, arg3, arg4;
    size_t payloadSize;
} asyncPacket;
```

**Subscriber Registration:**
```c
// 각 activeObject는 생성 시 eventId 범위를 구독 등록
asyncSubscribe(pActor, appMainEventStart, appMainEventEnd);
// 이후 asyncPush()는 eventId를 보고 자동으로 pActor의 큐에 적재
```

**Cross-Thread Message Flow:**
```
Sender Thread:
  asyncPush(asyncTypeAsync, RENDER_DRAW_FRAME, buf, size, 0, 0)
      ↓
  _asyncLookupTarget(RENDER_DRAW_FRAME) → renderActor
      ↓
  bufferPush(&renderActor->eventQueue[senderIdx], asyncPacket)
      ↓
  osalEpollNotify(&renderActor->objEpoll)  // [Linux]
  osalSemaphoreGive(&renderActor->objSema)  // [Win32]
      ↓
Receiver Thread (event loop):
  epoll_wait → eventFd → asyncPop → appThreadHandler()
```

### Ring Buffer

Lock-free Single-Producer Single-Consumer 링 버퍼입니다. 내부적으로 memory barrier를 사용하여 명시적인 lock 없이 thread-safe하게 동작합니다.

```c
typedef struct ringBuffer {
    uint8_t* pBuf;
    volatile size_t size, head, tail;
} ringBuffer;
```

### Logging System

Compile-time에 로그 수준과 백엔드를 선택합니다. 매크로를 통해 호출부에서 완전히 제거되거나(`#if APP_LOG_LEVEL >= ...`) 활성화됩니다.

```c
// Compile-time gated logging macros
logError("something broke: %d", err);   // 항상 출력 (level 1)
logWarn("unexpected: %s", msg);         // level 2
logInfo("initialized");                 // level 3
logDebug("counter: %lu", val);          // level 4

// Configurable features
#define APP_LOG_LEVEL     SYSTEM_LOG_LEVEL_DEBUG  // 출력 수준
#define APP_LOG_BACKEND   SYSTEM_LOG_BACKEND_PRINTF // printf/UART/printk
#define APP_LOG_COLOR     SYSTEM_LOG_COLOR_ENABLE  // ANSI color
#define APP_LOG_TIMESTAMP SYSTEM_LOG_TIMESTAMP_ENABLE // timestamp
```

```
[Example Output]
12:34:56.789 | app.c:42 [MARIO][INFO] system initialized
12:34:56.790 | physics.c:128 [MARIO][DEBUG] body created: id=7
```

---

## Project Structure

```
myLittleOS/
├── inc/                              # Public headers
│   ├── appCfgSelector.h              # Central config router (-DAPP_TARGET)
│   ├── app/
│   │   ├── sample/appConfig.h        # Linux, 2 threads, no driver
│   │   ├── engine2D/appConfig.h      # Win32, 2 threads, OpenGL
│   │   └── blackholeSimulation/      # Win32, 1 thread, bgfx+Jolt
│   ├── core/
│   │   ├── system.h                  # systemOpen / systemClose
│   │   ├── systemDefs.h              # System-wide defines, enums, config
│   │   └── feature/
│   │       ├── osal.h                # OSAL API (thread/mutex/sema/timer/epoll/mem)
│   │       ├── log.h                 # Logging macros (level-gated)
│   │       ├── async.h               # Async messaging (pub/sub)
│   │       ├── active.h              # Active Object (actor model)
│   │       └── buffer.h              # Ring Buffer
│   ├── driver/
│   │   ├── driverCommon.h / driverDefs.h
│   │   ├── platform/win32/
│   │   └── gfx/{opengl,bgfx}/
│   └── service/
│       ├── serviceCommon.h / serviceDefs.h
│       └── rendering/
├── src/                              # Implementation
│   ├── main.c                        # Entry point: systemOpen()
│   ├── core/
│   │   ├── system.c                  # systemOpen() init orchestration
│   │   ├── feature/
│   │   │   ├── osal.c                # 619 lines — full OSAL impl
│   │   │   ├── log.c                 # Log impl with mutex guard
│   │   │   ├── buffer.c              # Lock-free ring buffer
│   │   │   ├── async.c              # Event router (subscribe/push/pop)
│   │   │   └── active.c             # Actor thread lifecycle
│   │   └── physics/                  # Custom 2D physics engine
│   │       └── (vector/body/shape/contact/collision/solver/world)
│   ├── driver/                       # Platform, GFX, Physics drivers
│   ├── service/                      # Rendering 2D / 3D services
│   └── app/                          # 3 example applications
├── external/                         # Third-party dependencies
│   ├── bgfx/                         # bgfx (prebuilt .a + headers)
│   ├── Jolt/                         # Jolt Physics (full source)
│   └── par/                          # par_shapes (single-header)
├── assets/
│   ├── shaders/                      # GLSL + precompiled .bin
│   └── textures/                     # DDS cubemap, HDR env map
├── CMakeLists.txt                    # Build system (155 lines)
└── main.c                            # Entry point
```

---

## Getting Started

### Prerequisites

- CMake >= 3.10
- C11 / C++20 compiler (GCC or Clang)
- Linux: pthreads, librt
- Win32: Windows SDK (gdi32, user32, etc.)

### Build

```bash
# Clone
git clone https://github.com/username/myLittleOS
cd myLittleOS

# Sample App (Linux, 2-thread async messaging test)
cmake -DAPP_TARGET=sample -B build
cmake --build build

# 2D Physics Engine (Win32 + OpenGL)
cmake -DAPP_TARGET=engine_2d -B build
cmake --build build

# 3D Blackhole Simulation (Win32 + bgfx + Jolt Physics)
cmake -DAPP_TARGET=blackhole_simulation -B build
cmake --build build
```

### Quick Build Test

```bash
mkdir -p build && cmake -DAPP_TARGET=sample -B build && cmake --build build && ./build/myLittleOS
```

### Adding a New Application

```c
// 1. Create inc/app/myapp/appConfig.h
#define APP_OS OS_LINUX
#define APP_THREAD_MAX_COUNT 2
// ... define platform, drivers, services, log, memory, osal configs

// 2. Create src/app/myapp/app.c
int appOpen(void) {
    static activeObject actors[APP_THREAD_MAX_COUNT];
    // Initialize actor fields
    actors[0].isMainThread = true;
    actors[0].appThreadHandler = myMainEventHandler;
    actors[0].appTimerHandler = myMainTimerHandler;
    // ...
    activeOpen(&actors[0]);  // thread create + subscribe + timer start
    return 0;
}

// 3. Register in CMakeLists.txt
//    Add elseif(APP_TARGET STREQUAL "myapp")
//    with source files and link libraries

// 4. Build
cmake -DAPP_TARGET=myapp -B build && cmake --build build
```

### Adding a New Driver

```c
// 1. Define type in inc/driver/driverDefs.h
#define DRIVER_XXX 3

// 2. Implement src/driver/xxx/driverXxx.c
int driverXxxOpen(void);
int driverXxxClose(void);
int driverXxxSync(uint16_t sync, uintptr_t a1, uintptr_t a2, uintptr_t a3, uintptr_t a4);

// 3. Register in src/driver/driverCommon.c
int driverCommonOpen(void) {
#if APP_DRIVER_XXX
    if(driverXxxOpen()) return -1;
#endif
    // ...
}
```

---

## Example Applications

### sample

| Item | Description |
|------|-------------|
| **Purpose** | Active Object + Async Messaging 기능 검증 |
| **Platform** | Linux (pthreads + epoll) |
| **Threads** | Main + Test (2 active objects) |
| **Timer** | 500ms interval (main thread) |
| **Significance** | 프레임워크 최소 동작 단위 검증. 두 액터 간 async 메시지 송수신 확인 |

```c
// sample architecture
activeObject mainActor (isMainThread, timer=500ms) ←→ async messages ←→ activeObject testActor
```

### engine_2d

| Item | Description |
|------|-------------|
| **Purpose** | Driver/Service 계층 분리 + 2-thread Active Object 협업 검증 |
| **Platform** | Win32 (OpenGL) |
| **Threads** | Main(physics) + Render(OpenGL) |
| **Features** | Custom 2D physics engine, OpenGL fixed-function rendering |
| **Significance** | Core 계층의 custom 2D 물리 엔진 → Driver(OpenGL) → Service(Rendering2D) → App 전반의 데이터 흐름 검증 |

### blackhole_simulation

| Item | Description |
|------|-------------|
| **Purpose** | 외부 라이브러리 Driver 통합 + 3D Rendering Service 검증 |
| **Platform** | Win32 (bgfx + Jolt Physics) |
| **Threads** | Single (main) |
| **Features** | 3D camera control (FPS-style), gravitational lensing GLSL shader |
| **Significance** | bgfx, Jolt Physics 등 실제 외부 라이브러리를 Driver 계층으로 추상화하여 통합하는 방법론 검증 |

---

## Development Status

본 프로젝트는 **핵심 아키텍처의 안정화와 설계 검증을 완료**하였습니다.

**Completed:**
- Core: OSAL (Linux pthreads/epoll + Win32), Active Object, Async Messaging, Ring Buffer, Logging
- Driver/Service 계층 구조 및 dispatch 시스템
- Custom 2D Physics Engine (body/shape/collision/solver/world)
- 외부 라이브러리 통합 (bgfx, Jolt Physics via Driver abstraction)
- 3가지 예제 애플리케이션을 통한 architecture 검증

**Current Status:**
- 그래픽 관련 기능(OpenGL, bgfx)과 물리 엔진(Jolt) 연동은 **프레임워크 아키텍처의 확장성을 검증하기 위한 프로토타입 수준**입니다.
- 이들은 프로젝트의 주 목적이 아닌 **응용 예시**에 해당하며, 단순한 도형 렌더링 이상의 그래픽스 기능을 포함하지 않습니다.

**Planned (중단):**
- `asyncTypeAwait` / `asyncTypeExpress` 구현
- Async log 모드
- Embedded 타겟 (FreeRTOS, Zephyr) 포팅
- 추가 Driver/Service 개발

프로젝트의 추가 개발은 현재 중단된 상태이며, 필요 시 재개할 수 있습니다.

---

## Keywords

`C` `C++20` `CMake` `Active Object` `Actor Pattern` `OS Abstraction Layer`
`Concurrency` `Lock-Free` `Ring Buffer` `Async Messaging` `Embedded Ready`
`Cross Platform` `Linux` `Win32` `bgfx` `Jolt Physics`

---

*Author: Minkyu Kim*
