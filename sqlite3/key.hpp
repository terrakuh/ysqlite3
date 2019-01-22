#pragma once

#include <cstddef>
#include <string>

#include "base64.hpp"


namespace ysqlite3
{

typedef std::string key_t;

namespace key_literals
{

inline key_t operator""_key(const char * _value, size_t _size)
{
	return base64::encode(_value, _size);
}

inline key_t operator""_hex(const char * _value, size_t _size)
{
	throw;
}

}
}