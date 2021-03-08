cmake_minimum_required(VERSION 3.1)

add_subdirectory(../3rd_party/Catch2 "${CMAKE_CURRENT_BINARY_DIR}/3rd_party/Catch2")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/3rd_party/Catch2/contrib")
include(CTest)
include(Catch)
enable_testing()

add_executable(crypt-vfs "crypt_vfs.cpp")
target_link_libraries(crypt-vfs PUBLIC Catch2::Catch2 ysqlite3::ysqlite3)
catch_discover_tests(crypt-vfs)
