cmake_minimum_required(VERSION 3.1)

project(ysqlite3
	VERSION 0.1.0
	LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 11)

option(YSQLITE3_USE_OPENSSL "Build OpenSSL encryptors." OFF)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O -Wall -Wextra -Wuninitialized -fno-omit-frame-pointer -fsanitize=undefined,float-divide-by-zero,integer,nullability -g")

file(GLOB_RECURSE sources
	"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(ysqlite3 "${sources}" "${CMAKE_CURRENT_SOURCE_DIR}/src/sqlite3.c")
add_library(ysqlite3::ysqlite3 ALIAS ysqlite3)

add_executable(main "${CMAKE_CURRENT_SOURCE_DIR}/examples/main.cpp")
target_link_libraries(main ysqlite3)

install(TARGETS ysqlite3
	EXPORT ysqlite3
	ARCHIVE DESTINATION "lib"
	LIBRARY DESTINATION "lib")
install(DIRECTORY "ysqlite3"
	DESTINATION "include")
install(DIRECTORY "3rd_party"
	DESTINATION "include/ysqlite3")
install(EXPORT ysqlite3
	DESTINATION "lib/cmake/ysqlite3"
	NAMESPACE ysqlite3::
	FILE "ysqlite3-config.cmake")
