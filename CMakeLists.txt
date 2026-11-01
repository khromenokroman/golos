cmake_minimum_required(VERSION 3.16)

project(golos VERSION 0.1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.26
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(miniaudio)

set(WHISPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    whisper
    GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
    GIT_TAG v1.9.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(whisper)

add_library(golos_core STATIC
    src/recorder.cpp
    src/transcriber.cpp
    src/typer.cpp
    src/wav.cpp
)

target_include_directories(golos_core PUBLIC src ${miniaudio_SOURCE_DIR})
target_link_libraries(golos_core PUBLIC whisper)

add_executable(golos src/main.cpp)
target_link_libraries(golos PRIVATE golos_core)
target_compile_options(golos_core PRIVATE -Wall -Wextra)
target_compile_options(golos PRIVATE -Wall -Wextra)

option(GOLOS_BUILD_TESTS "Собирать тесты" ON)
if(GOLOS_BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    add_executable(golos_tests
        tests/test_wav.cpp
        tests/test_text_util.cpp
    )
    target_link_libraries(golos_tests PRIVATE golos_core gtest_main)

    include(GoogleTest)
    gtest_discover_tests(golos_tests)
endif()
