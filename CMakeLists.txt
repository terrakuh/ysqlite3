cmake_minimum_required(VERSION 3.16)

project(
  ysqlite3
  VERSION 0.5.0
  LANGUAGES C CXX
)

set(YSQLITE3_IS_TOP_LEVEL OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(YSQLITE3_IS_TOP_LEVEL ON)
endif()

option(YSQLITE3_BUILD_EXAMPLES "Build the example projects." ${YSQLITE3_IS_TOP_LEVEL})
option(YSQLITE3_BUILD_EXTENSIONS "Build the extensions." ${YSQLITE3_IS_TOP_LEVEL})
option(YSQLITE3_BUILD_TESTS "Build the tests." ${YSQLITE3_IS_TOP_LEVEL})
option(YSQLITE3_BUILD_SHELL "Build the shell." ${YSQLITE3_IS_TOP_LEVEL})
option(YSQLITE3_ENABLE_JSON "Enables the JSON extension for the shell." ON)
option(YSQLITE3_ENABLE_EXPLAIN_COMMENTS "Enables extra commentary." ON)
option(YSQLITE3_ENABLE_RTREE "Enables the rtree extension for the shell." ON)
set(YSQLITE3_ENABLE_FULL_TEXT_SEARCH
    "FTS5"
    CACHE STRING "Enables and sets the FTS version."
)
set_property(CACHE YSQLITE3_ENABLE_FULL_TEXT_SEARCH PROPERTY STRINGS "no" "FTS3" "FTS4" "FTS5")
set(YSQLITE3_ENCRYPTION_BACKEND
    "OpenSSL"
    CACHE STRING "The backend library for encryption/decryption ('none', 'OpenSSL')."
)
set_property(CACHE YSQLITE3_ENCRYPTION_BACKEND PROPERTY STRINGS "none" "OpenSSL")
set(YSQLITE3_CRYPT_VFS_NAME
    "ysqlite3-crypt"
    CACHE STRING "The name of the crypt VFS."
)
# add_compile_options("-DPRINT_DEBUG")

find_package(Threads REQUIRED)

if("${YSQLITE3_ENCRYPTION_BACKEND}" STREQUAL "OpenSSL")
  set(YSQLITE3_ENCRYPTION_BACKEND_OPENSSL TRUE)
endif()

# endian
include(TestBigEndian)
test_big_endian(YSQLITE3_BIG_ENDIAN)

# configure
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in" "${CMAKE_CURRENT_SOURCE_DIR}/ysqlite3/config.hpp")

file(GLOB_RECURSE sources "${CMAKE_CURRENT_SOURCE_DIR}/ysqlite3/*.cpp")

add_library(ysqlite3 "${sources}" "${CMAKE_CURRENT_SOURCE_DIR}/ysqlite3/sqlite3.c")
add_library(ysqlite3::ysqlite3 ALIAS ysqlite3)
set_target_properties(ysqlite3 PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(
  ysqlite3 PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
                  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>"
)
target_link_libraries(ysqlite3 PUBLIC Threads::Threads "${CMAKE_DL_LIBS}")
target_compile_features(ysqlite3 PUBLIC cxx_std_20)

# json
if(YSQLITE3_ENABLE_JSON)
  target_compile_definitions(ysqlite3 PRIVATE SQLITE_ENABLE_JSON1)
endif()

# comments
if(YSQLITE3_ENABLE_EXPLAIN_COMMENTS)
  target_compile_definitions(ysqlite3 PRIVATE SQLITE_ENABLE_EXPLAIN_COMMENTS)
endif()

# rtree
if(YSQLITE3_ENABLE_RTREE)
  target_compile_definitions(ysqlite3 PRIVATE SQLITE_ENABLE_RTREE)
endif()

# fts
if(NOT YSQLITE3_ENABLE_FULL_TEXT_SEARCH STREQUAL "no")
  # target_compile_definitions(ysqlite3 PRIVATE SQLITE_ENABLE_FTS5)
  target_compile_definitions(ysqlite3 PRIVATE SQLITE_ENABLE_${YSQLITE3_ENABLE_FULL_TEXT_SEARCH})
endif()

# math
if(YSQLITE3_ENABLE_MATH_FUNCTIONS)
  target_compile_definitions(ysqlite3 PRIVATE SQLITE_ENABLE_MATH_FUNCTIONS)
endif()

# encryption
if(YSQLITE3_ENCRYPTION_BACKEND_OPENSSL)
  find_package(OpenSSL REQUIRED COMPONENTS Crypto)
  target_link_libraries(ysqlite3 PUBLIC OpenSSL::Crypto)
endif()

if(YSQLITE3_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(YSQLITE3_BUILD_EXTENSIONS)
  add_subdirectory(extensions)
endif()

if(YSQLITE3_BUILD_SHELL)
  add_subdirectory(shell)
endif()

if(YSQLITE3_BUILD_TESTS)
  add_subdirectory(tests)
endif()

# install
install(TARGETS ysqlite3 EXPORT ysqlite3)
install(
  DIRECTORY "ysqlite3"
  DESTINATION "include"
  FILES_MATCHING
  REGEX ".*\.h(pp)?$"
)
install(
  EXPORT ysqlite3
  DESTINATION "lib/cmake/ysqlite3"
  NAMESPACE ysqlite3::
  FILE "ysqlite3-config.cmake"
)

# CPack
if(YSQLITE3_IS_TOP_LEVEL)
  set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
  set(CPACK_PACKAGE_CONTACT "Yunus Ayar")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}")
  set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
  set(CPACK_GENERATOR DEB TGZ)
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

  include(CPack)
endif()
