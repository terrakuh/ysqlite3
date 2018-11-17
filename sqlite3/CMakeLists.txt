cmake_minimum_required(VERSION 3.0)

set(YSQLITE3_HEADER_FILES "config.hpp" "database.hpp" "encrypted_database.hpp" "encrypted_vfs.hpp" "encryption_context.hpp" "exception.hpp" "handle.hpp" "no_encryption_context.hpp" "statement.hpp")
set(YSQLITE3_SOURCE_FILES "database.cpp" "encrypted_database.cpp" "encrypted_vfs.cpp" "handle.cpp" "no_encryption_context.cpp" "statement.cpp")
set(SQLITE3_SOURCE_FILES "sqlite3.c")

add_library(ysqlite3 ${YSQLITE3_SOURCE_FILES} ${SQLITE_SOURCE_FILES})
