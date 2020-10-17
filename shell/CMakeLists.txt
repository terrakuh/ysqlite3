cmake_minimum_required(VERSION 3.1)

add_executable(shell "shell.c" "entrypoint.cpp")
target_link_libraries(shell ysqlite3::ysqlite3)
target_include_directories(shell PRIVATE "../ysqlite3")
set_target_properties(shell PROPERTIES
	OUTPUT_NAME "ysqlite3"
	SUFFIX ""
	PREFIX "")

install(TARGETS shell
	RUNTIME DESTINATION "bin")
