cmake_minimum_required(VERSION 3.1)

find_package(Catch2 REQUIRED)

include(Catch)

add_executable(crypt-vfs "crypt_vfs.cpp")
target_link_libraries(crypt-vfs PUBLIC Catch2::Catch2 ysqlite3::ysqlite3)
catch_discover_tests(crypt-vfs)
