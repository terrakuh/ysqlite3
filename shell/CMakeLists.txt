cmake_minimum_required(VERSION 3.1)

option(YSQLITE3_SHELL_ENABLE_READLINE "Enables the use of the readline library." ON)

add_executable(shell "shell.c" "entrypoint.cpp")
target_link_libraries(shell PRIVATE ysqlite3::ysqlite3)
target_include_directories(shell PRIVATE "../ysqlite3")
set_target_properties(
  shell
  PROPERTIES OUTPUT_NAME "ysqlite3"
             SUFFIX ""
             PREFIX ""
)

# readline
if(YSQLITE3_SHELL_ENABLE_READLINE)
  find_library(READLINE_LIBRARY readline REQUIRED)
  find_path(READLINE_INCLUDE_DIR readline/readline.h REQUIRED)
  target_link_libraries(shell PRIVATE "${READLINE_LIBRARY}")
  target_include_directories(shell PRIVATE "${READLINE_INCLUDE_DIR}")
  target_compile_definitions(shell PRIVATE HAVE_READLINE)
endif()

install(TARGETS shell)
