#pragma once

#include <cstdint>
#include <cstring>
#include <vector>


namespace ysqlite3
{

typedef std::vector<int8_t> key_t;

namespace key_literals
{

key_t operator""_key(const char * _value, size_t _size)
{
	key_t _key;

	_key.resize(_size);

	std::memcpy(_key.data(), _value, _size);

	return _key;
}

key_t operator""_hex(const char * _value, size_t _size)
{
	throw;
}

}
}