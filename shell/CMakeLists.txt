cmake_minimum_required(VERSION 3.1)

option(YSQLITE3_SHELL_DISABLE_READLINE "Disables the use of the readline library." OFF)

add_executable(shell "shell.c" "entrypoint.cpp")
target_link_libraries(shell ysqlite3::ysqlite3)
target_include_directories(shell PRIVATE "../ysqlite3")
set_target_properties(shell PROPERTIES
	OUTPUT_NAME "ysqlite3"
	SUFFIX ""
	PREFIX "")

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
