add_library(session_lib STATIC
    PlayerSession.cpp
)

target_include_directories(session_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
