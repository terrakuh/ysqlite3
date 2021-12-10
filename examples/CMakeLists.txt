add_executable(simple "simple.cpp")
target_link_libraries(simple PRIVATE ysqlite3::ysqlite3)

add_executable(rot13 "rot13.cpp")
target_link_libraries(rot13 PRIVATE ysqlite3::ysqlite3)

if(NOT "${YSQLITE3_ENCRYPTION_BACKEND}" STREQUAL "none")
  add_executable(encryption "encryption.cpp")
  target_link_libraries(encryption PRIVATE ysqlite3::ysqlite3)
endif()
