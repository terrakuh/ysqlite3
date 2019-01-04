#pragma once

#include <cstdint>
#include <cstddef>


namespace ysqlite3
{

class encryption_context
{
public:
	virtual bool decrypt(uint32_t _id, void * _buffer, size_t _size) noexcept = 0;
	virtual const void * encrypt(uint32_t _id, const void * _buffer, size_t _size, void * _output) noexcept = 0;
};

}
