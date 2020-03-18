cmake_minimum_required(VERSION 3.1)

project(ysqlite3)
set(CMAKE_CXX_STANDARD 14)

option(YSQLITE3_USE_OPENSSL "Build OpenSSL encryptors." OFF)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O -Wall -Wextra -Wuninitialized -fno-omit-frame-pointer -fsanitize=undefined,float-divide-by-zero,integer,nullability -g")
find_package(Threads REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "The build type." FORCE)
endif()

file(GLOB_RECURSE YSQLITE_SOURCE_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_library(ysqlite3 "${YSQLITE_SOURCE_FILES}" "${CMAKE_CURRENT_SOURCE_DIR}/src/sqlite3.c")
target_include_directories(ysqlite3
							PUBLIC 
								"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
								"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ysqlite3>"
								"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/3rd_party>"
								"$<INSTALL_INTERFACE:include>"
								"$<INSTALL_INTERFACE:include/ysqlite3/3rd_party>")
target_link_libraries(ysqlite3
						PUBLIC Threads::Threads
						PUBLIC "${CMAKE_DL_LIBS}")

add_executable(main "${CMAKE_CURRENT_SOURCE_DIR}/examples/main.cpp")
target_link_libraries(main ysqlite3)

install(TARGETS ysqlite3
		EXPORT ysqlite3
		ARCHIVE
			DESTINATION "lib"
		LIBRARY
			DESTINATION "lib")
install(DIRECTORY "ysqlite3"
		DESTINATION "include")
install(DIRECTORY "3rd_party"
		DESTINATION "include/ysqlite3")
install(EXPORT ysqlite3
		DESTINATION "lib/cmake/ysqlite3"
		NAMESPACE ysqlite3::
		FILE "ysqlite3-config.cmake")