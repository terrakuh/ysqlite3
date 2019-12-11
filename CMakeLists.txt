cmake_minimum_required(VERSION 3.1)

project(ysqlite3)
set(CMAKE_CXX_STANDARD 11)

option(YSQLITE3_USE_OPENSSL "Build OpenSSL encryptors." OFF)


file(GLOB_RECURSE YSQLITE_SOURCE_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library("ysqlite3" ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories("ysqlite3"
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})