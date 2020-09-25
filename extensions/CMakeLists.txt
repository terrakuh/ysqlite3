cmake_minimum_required(VERSION 3.1)
project(ysqlite3-extensions
	LANGUAGES CXX)

find_package(OpenSSL REQUIRED COMPONENTS Crypto)

add_library(ysqlite3_crypt_vfs SHARED "ysqlite3_crypt_vfs.cpp")
target_link_libraries(ysqlite3_crypt_vfs
	PUBLIC
		OpenSSL::Crypto
		ysqlite3::ysqlite3)
set_target_properties(ysqlite3_crypt_vfs PROPERTIES
	POSITION_INDEPENDENT_CODE TRUE
	PREFIX ""
	SUFFIX ".ext")
