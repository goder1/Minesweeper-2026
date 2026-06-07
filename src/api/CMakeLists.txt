add_library(api_lib STATIC
    JwtHelper.cpp
    AuthController.cpp
    GameController.cpp
    LeaderboardController.cpp
    UserController.cpp
)

target_include_directories(api_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(api_lib PUBLIC
    app_lib
    db_lib
    Drogon::Drogon
    unofficial-sodium::sodium
    jwt-cpp::jwt-cpp
)
