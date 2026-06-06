add_library(app_lib STATIC
    App.cpp
)

target_include_directories(app_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(app_lib PUBLIC game_logic session_lib)
