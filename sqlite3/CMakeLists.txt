cmake_minimum_required(VERSION 3.0)

set(YSQLITE3_HEADER_FILES "config.hpp" "database.hpp" "encrypted_database.hpp" "encrypted_vfs.hpp" "encryption_context.hpp" "exception.hpp" "handle.hpp" "no_encryption_context.hpp" "statement.hpp")
set(YSQLITE3_SOURCE_FILES "database.cpp" "encrypted_database.cpp" "encrypted_vfs.cpp" "handle.cpp" "no_encryption_context.cpp" "statement.cpp")
set(SQLITE3_SOURCE_FILES "sqlite3.c")

add_library(ysqlite3 ${YSQLITE3_SOURCE_FILES} ${SQLITE_SOURCE_FILES})

include(CMakePackageConfigHelpers)
write_basic_package_version_file("ysqlite3Config.cmake" VERSION 1.0 COMPATIBILITY AnyNewerVersion)

install(TARGETS ysqlite3
  EXPORT ysqlite3Config
  ARCHIVE DESTINATION lib/ysqlite3
  LIBRARY DESTINATION lib/ysqlite3
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include/ysqlite3
)
install(FILES ${YSQLITE3_HEADER_FILES}
  DESTINATION include/ysqlite3
)
export(TARGETS ysqlite3
  FILE "ysqlite3Config.cmake"
)
install(EXPORT ysqlite3Config
  DESTINATION "lib/ysqlite3/cmake"
)
