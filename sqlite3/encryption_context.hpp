#pragma once

#include <cstdint>
#include <vector>
#include <tuple>
#include <functional>


namespace sqlite3
{

class encryption_context
{
public:
	virtual bool encrypt(uint32_t _id, const void * _buffer, size_t _size, void * _output) noexcept = 0;
	virtual bool decrypt(uint32_t _id, void * _buffer, size_t _size) noexcept = 0;
	virtual bool does_something() const noexcept = 0;
	virtual size_t required_output_size(size_t _input) const noexcept = 0;
};

}