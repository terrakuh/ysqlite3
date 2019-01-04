#pragma once

#include "encryption_context.hpp"


namespace ysqlite3
{

class no_encryption_context : public encryption_context
{
public:
	virtual bool encrypt(uint32_t _id, const void * _buffer, size_t _size, void * _output) noexcept override
	{
		return false;
	}
	virtual bool decrypt(uint32_t _id, void * _buffer, size_t _size) noexcept override
	{
		return false;
	}
	virtual bool does_something() const noexcept override
	{
		return false;
	}
	virtual size_t required_output_size(size_t _input) const noexcept override
	{
		return _input;
	}
};

}