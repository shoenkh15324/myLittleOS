cmake_minimum_required(VERSION 3.10)
project(Dirac_Os)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-fdiagnostics-color=always)
endif()

add_executable(dirac_daemon 
    main.c
    # Core
    src/core/system.c
    src/core/osal.c
    src/core/log.c
    src/core/buffer.c
    src/core/async.c
    # Driver
    src/driver/driverCommon.c
    # Service
    src/service/serviceCommon.c
    # App
    src/app/appCommon.c
    src/app/sample/app.c
    src/app/sample/appBridge.c
)

target_include_directories(dirac_daemon
    PRIVATE
        ${CMAKE_SOURCE_DIR}/inc
)

target_compile_definitions(dirac_daemon PUBLIC
    DIRAC_OS=1
    APP_SAMPLE=1
)