cmake_minimum_required(VERSION 3.1)

option(YSQLITE3_SHELL_ENABLE_JSON "Enables the JSON extension for the shell." ON)
option(YSQLITE3_SHELL_ENABLE_EXPLAIN_COMMENTS "Enables extra commentary." ON)
option(YSQLITE3_SHELL_ENABLE_RTREE "Enables the rtree extension for the shell." ON)
option(YSQLITE3_SHELL_DISABLE_READLINE "Disables the use of the readline library." OFF)
set(YSQLITE3_SHELL_ENABLE_FULL_TEXT_SEARCH "FTS5" CACHE STRING "Enables and sets the FTS version.")
set_property(CACHE YSQLITE3_SHELL_ENABLE_FULL_TEXT_SEARCH
	PROPERTY STRINGS "no" "FTS3" "FTS4" "FTS5")

add_executable(shell "shell.c" "entrypoint.cpp")
target_link_libraries(shell ysqlite3::ysqlite3)
target_include_directories(shell PRIVATE "../ysqlite3")
set_target_properties(shell PROPERTIES
	OUTPUT_NAME "ysqlite3"
	SUFFIX ""
	PREFIX "")

# json
if(YSQLITE3_SHELL_ENABLE_JSON)
	target_compile_definitions(shell PRIVATE SQLITE_ENABLE_JSON1)
endif()

# comments
if(YSQLITE3_SHELL_ENABLE_EXPLAIN_COMMENTS)
	target_compile_definitions(shell PRIVATE SQLITE_ENABLE_EXPLAIN_COMMENTS)
endif()

# rtree
if(YSQLITE3_SHELL_ENABLE_RTREE)
	target_compile_definitions(shell PRIVATE SQLITE_ENABLE_RTREE)
endif()

# fts
if(NOT YSQLITE3_SHELL_ENABLE_FULL_TEXT_SEARCH STREQUAL "no")
	target_compile_definitions(shell PRIVATE SQLITE_ENABLE_${YSQLITE3_SHELL_ENABLE_FULL_TEXT_SEARCH})
endif()

# readline
find_library(READLINE_LIBRARY readline)
find_path(READLINE_INCLUDE_DIR readline/readline.h)

if(NOT YSQLITE3_SHELL_DISABLE_READLINE AND READLINE_INCLUDE_DIR AND READLINE_LIBRARY)
	message(STATUS "Found readline ${READLINE_INCLUDE_DIR} AND ${READLINE_LIBRARY}")
	target_link_libraries(shell "${READLINE_LIBRARY}")
	target_compile_definitions(shell PRIVATE HAVE_READLINE)
endif()

install(TARGETS shell
	RUNTIME DESTINATION "bin")
