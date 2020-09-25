cmake_minimum_required(VERSION 3.1)

project(ysqlite3
	VERSION 0.1.0
	LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 11)

option(YSQLITE3_BUILD_EXAMPLES "Build the example projects." ON)
set(YSQLITE3_ENCRYPTION_BACKEND "OpenSSL"
	CACHE STRING "The backend library for encryption/decryption ('none', 'OpenSSL').")
set_property(CACHE YSQLITE3_ENCRYPTION_BACKEND PROPERTY
	STRINGS "none" "OpenSSL")

find_package(ZLIB)
find_package(Threads REQUIRED)

set(YSQLITE3_ZLIB_FOUND ${ZLIB_FOUND})

if("${YSQLITE3_ENCRYPTION_BACKEND}" STREQUAL "OpenSSL")
	set(YSQLITE3_ENCRYPTION_BACKEND_OPENSSL TRUE)
endif()

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in"
	"${CMAKE_CURRENT_SOURCE_DIR}/ysqlite3/config.hpp")

file(GLOB_RECURSE sources
	"${CMAKE_CURRENT_SOURCE_DIR}/ysqlite3/*.cpp")

add_library(ysqlite3 "${sources}" "${CMAKE_CURRENT_SOURCE_DIR}/ysqlite3/sqlite3.c")
add_library(ysqlite3::ysqlite3 ALIAS ysqlite3)
target_include_directories(ysqlite3
	PUBLIC
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
		"$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>")
target_link_libraries(ysqlite3
	PUBLIC
		Threads::Threads
		"${CMAKE_DL_LIBS}")

if(ZLIB_FOUND)
	target_link_libraries(ysqlite3 PUBLIC ZLIB::ZLIB)
else()
	message(WARN "ZLib not found. Not building compression VFS.")
endif()

if(YSQLITE3_ENCRYPTION_BACKEND_OPENSSL)
	find_package(OpenSSL REQUIRED COMPONENTS Crypto)
	target_link_libraries(ysqlite3 PUBLIC OpenSSL::Crypto)
endif()

if(YSQLITE3_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()

# install
install(TARGETS ysqlite3
	EXPORT ysqlite3
	ARCHIVE DESTINATION "lib"
	LIBRARY DESTINATION "lib")
install(DIRECTORY "ysqlite3"
	DESTINATION "include"
	FILES_MATCHING REGEX "*.h(pp)?$")
install(EXPORT ysqlite3
	DESTINATION "lib/cmake/ysqlite3"
	NAMESPACE ysqlite3::
	FILE "ysqlite3-config.cmake")
