cmake_minimum_required(VERSION 3.1)

project(ysqlite3_examples
	LANGUAGES CXX)

add_executable(simple "simple.cpp")
target_link_libraries(simple ysqlite3::ysqlite3)

add_executable(rot13 "rot13.cpp")
target_link_libraries(rot13 ysqlite3::ysqlite3)

add_executable(encryption "encryption.cpp")
target_link_libraries(encryption ysqlite3::ysqlite3)
