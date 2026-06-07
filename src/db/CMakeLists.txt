add_library(db_lib STATIC
    DbClient.cpp
    UserRepository.cpp
    RecordRepository.cpp
    GameRepository.cpp
)

target_include_directories(db_lib PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(db_lib PUBLIC Drogon::Drogon)
