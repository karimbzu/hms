cmake_minimum_required(VERSION 3.10)
project(crow_sqlite_crud)

set(CMAKE_CXX_STANDARD 17)
include_directories(include)

add_executable(crow_sqlite_crud main.cpp)
target_link_libraries(crow_sqlite_crud sqlite3 pthread)

