add_library(game_logic STATIC
    Cell.cpp
    Board.cpp
    Difficulty.cpp
    GameSession.cpp
)

target_include_directories(game_logic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
