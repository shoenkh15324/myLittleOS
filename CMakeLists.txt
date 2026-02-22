cmake_minimum_required(VERSION 3.10)
project(myLittleOS LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 1. 프로젝트 설정 (App Target)
if(NOT DEFINED APP_TARGET)
    set(APP_TARGET "euclid_engine")
endif()

set(TARGET_NAME myLittleOS)
set(APP_DEFINE "APP_${APP_TARGET}")
set(EXTERNAL_DIR "${CMAKE_SOURCE_DIR}/external")
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

# 4. 소스 파일 정의
set(COMMON_SRCS
    main.c
    # Core
    src/core/system.c
    src/core/feature/osal.c
    src/core/feature/log.c
    src/core/feature/buffer.c
    src/core/feature/async.c
    src/core/feature/active.c
    # Driver
    src/driver/driverCommon.c
    # Service
    src/service/serviceCommon.c
    # App
    src/app/appCommon.c
)
# 4-1. 타겟별 소스
if(APP_TARGET STREQUAL "engine_2d")
    set(APP_SRCS
        # core
        src/core/physics/vector/vector2D.c
        src/core/physics/body/body2D.c
        src/core/physics/shape/shape2D.c
        src/core/physics/shape/circle2D.c
        src/core/physics/shape/aabb2D.c
        src/core/physics/contact/contact2D.c
        src/core/physics/collision/collision2D.c
        src/core/physics/solver/solver2D.c
        src/core/physics/world/world2D.c
        # driver
        src/driver/platform/win32/driverPlatformWin32.c
        src/driver/gfx/opengl/driverOpengl.c
        # service
        src/service/rendering/serviceRendering2D.c
        # app
        src/app/engine2D/app.c
        src/app/engine2D/appBridge.c
    )
    list(APPEND EXTERNAL_LIBS opengl32 gdi32 user32)
elseif(APP_TARGET STREQUAL "euclid_engine")
    # jolt physic
    set(JOLT_ROOT_DIR "${EXTERNAL_DIR}/Jolt")
    file(GLOB_RECURSE JOLT_SRCS "${JOLT_ROOT_DIR}/*.cpp")
    # bgfx
    set(BGFX_ROOT_DIR ${EXTERNAL_DIR}/bgfx)
    # myLittleOS
    set(APP_SRCS
        ${JOLT_SRCS}
        ${BGFX_SRCS}
        # driver
        src/driver/platform/win32/driverPlatformWin32.c
        src/driver/gfx/bgfx/driverBgfx.c
        src/driver/gfx/bgfx/driverBgfxBridge.cpp
        src/driver/physicsBackend/jolt/driverJolt.c
        src/driver/physicsBackend/jolt/driverJoltBridge.cpp
        # service
        src/service/rendering/serviceRendering3D.c
        # app
        src/app/euclidEngine/app.c
        src/app/euclidEngine/appBridge.c     
    )
    set(JOLT_DEFINITIONS # Jolt용 컴파일 정의 추가
        JPH_DEBUG 
        JPH_PROFILE 
        JPH_FLOATING_POINT_EXCEPTIONS_ENABLED
    )
    set(BGFX_DEFINITIONS
        __STDC_FORMAT_MACROS
        BGFX_CONFIG_RENDERER_OPENGL=1
        BX_PLATFORM_WINDOWS=1
        BX_CONFIG_DEBUG=0
    )
    link_directories("${BGFX_ROOT_DIR}/lib")
    list(APPEND EXTERNAL_LIBS bgfxRelease bimgRelease bxRelease gdi32 user32 psapi dxgi d3d11 d3d12 dwmapi stdc++)
    if(WIN32)
        list(APPEND JOLT_DEFINITIONS JPH_PLATFORM_WINDOWS)
    endif()
    file(COPY "${CMAKE_SOURCE_DIR}/assets" DESTINATION "${CMAKE_BINARY_DIR}")
else()
    message(FATAL_ERROR "Unknown APP_TARGET: ${APP_TARGET}")
endif()
# 5. 타겟 생성 및 설정
set(SRCS ${COMMON_SRCS} ${APP_SRCS})
add_executable(${TARGET_NAME} ${SRCS})
if(APP_TARGET STREQUAL "euclid_engine")
    target_precompile_headers(${TARGET_NAME} PRIVATE  # Jolt/Jolt.h를 미리 컴파일하여 빌드 속도를 비약적으로 향상.
        "$<$<COMPILE_LANGUAGE:CXX>:Jolt/Jolt.h>"
    )
endif()
# 6. 인클루드 경로 설정
target_include_directories(${TARGET_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/inc)
if(APP_TARGET STREQUAL "euclid_engine")
    target_include_directories(${TARGET_NAME} PRIVATE 
        "${EXTERNAL_DIR}"
        "${BGFX_ROOT_DIR}/include"
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(${TARGET_NAME} PRIVATE -fpermissive)
    endif()
    target_compile_definitions(${TARGET_NAME} PRIVATE
        ${JOLT_DEFINITIONS} ${BGFX_DEFINITIONS}
    )
    set_target_properties(${TARGET_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
    )
endif()

target_compile_definitions(${TARGET_NAME} PUBLIC
    MY_LITTLE_OS=1
    ${APP_DEFINE_UPPER}=1
)

# 7. 라이브러리 링크
target_link_libraries(${TARGET_NAME} PRIVATE ${EXTERNAL_LIBS})
