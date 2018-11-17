cmake_minimum_required(VERSION 3.1)

project(ysqlite3)
set(CMAKE_CXX_STANDARD 11)

find_package(Boost REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})

add_subdirectory(sqlite3)
