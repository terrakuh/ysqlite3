#pragma once

#include "encryption_context.hpp"


namespace ysqlite3
{

class no_encryption_context : public encryption_context
{
public:
	virtual bool decrypt(uint32_t _id, void * _buffer, size_t _size) noexcept override
	{
		return true;
	}
	virtual const void * encrypt(uint32_t _id, const void * _buffer, size_t _size, void * _output) noexcept override
	{
		return _buffer;
	}
};

}