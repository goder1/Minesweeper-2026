cmake_minimum_required(VERSION 3.20)
project(Minesweeper2026 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)
    if(CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    endif()
endif()

add_subdirectory(src/game)
add_subdirectory(src/session)
add_subdirectory(src/app)

add_executable(minesweeper src/main.cpp)
target_include_directories(minesweeper PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(minesweeper PRIVATE app_lib)

enable_testing()
add_subdirectory(tests/unit)
