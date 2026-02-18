cmake_minimum_required(VERSION 3.10)
project(myLittleOS LANGUAGES C CXX)

# 1. 프로젝트 설정 (App Target)
if(NOT DEFINED APP_TARGET)
    set(APP_TARGET "engine_2d")
endif()

set(TARGET_NAME myLittleOS)
set(APP_DEFINE "APP_${APP_TARGET}")
string(TOUPPER "${APP_DEFINE}" APP_DEFINE_UPPER)

message(STATUS "--------------------------------------------")
message(STATUS "Target Name: ${TARGET_NAME}")
message(STATUS "App Target : ${APP_TARGET} (${APP_DEFINE_UPPER})")
message(STATUS "--------------------------------------------")

# 2. 컴파일러 옵션
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fdiagnostics-color=always -Wall -Wextra)
endif()

# 3. 종속성 라이브러리 찾기
find_package(Threads REQUIRED)

set(EXTERNAL_LIBS Threads::Threads)
if(UNIX)
    find_library(RT_LIBRARY rt)
    if(RT_LIBRARY)
        list(APPEND EXTERNAL_LIBS ${RT_LIBRARY})
    endif()
endif()

# 4. 소스 파일 정의 (가독성을 위해 변수에 할당)
set(SRCS
    main.c
    # Core
    src/core/system.c
    src/core/feature/osal.c
    src/core/feature/log.c
    src/core/feature/buffer.c
    src/core/feature/async.c
    src/core/feature/active.c
    src/core/physics/vector/vector2D.c
    src/core/physics/body/body2D.c
    src/core/physics/shape/shape2D.c
    src/core/physics/shape/circle2D.c
    src/core/physics/shape/aabb2D.c
    src/core/physics/contact/contact2D.c
    src/core/physics/collision/collision2D.c
    src/core/physics/solver/solver2D.c
    src/core/physics/world/world2D.c
    # Driver
    src/driver/driverCommon.c
    src/driver/platform/win32/driverPlatformWin32.c
    src/driver/gfx/opengl/driverOpengl.c
    # Service
    src/service/serviceCommon.c
    # App
    src/app/appCommon.c
    src/app/engine2D/app.c
    src/app/engine2D/appBridge.c
)

# 5. 타겟 생성 및 설정
add_executable(${TARGET_NAME} ${SRCS})

target_include_directories(${TARGET_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/inc
)

target_compile_definitions(${TARGET_NAME} PUBLIC
    MY_LITTLE_OS=1
    ${APP_DEFINE_UPPER}=1
)

target_link_libraries(${TARGET_NAME} PRIVATE ${EXTERNAL_LIBS})
