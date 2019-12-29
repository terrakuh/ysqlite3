cmake_minimum_required(VERSION 3.1)

project(ysqlite3)
set(CMAKE_CXX_STANDARD 14)

option(YSQLITE3_USE_OPENSSL "Build OpenSSL encryptors." OFF)

find_package(Threads)

file(GLOB_RECURSE YSQLITE_SOURCE_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library("ysqlite3" ${YSQLITE_SOURCE_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/src/sqlite3.c")
target_include_directories("ysqlite3"
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/")
target_link_libraries("ysqlite3"
	PUBLIC Threads::Threads
	PUBLIC ${CMAKE_DL_LIBS})

add_executable("main" "${CMAKE_CURRENT_SOURCE_DIR}/examples/main.cpp")
target_link_libraries("main" PUBLIC "ysqlite3")