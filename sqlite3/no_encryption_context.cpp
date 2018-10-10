#include "no_encryption_context.hpp"


namespace sqlite3
{

bool no_encryption_context::encrypt(uint32_t _id, const void * _buffer, size_t _size, void * _output) noexcept
{
	return false;
}

bool no_encryption_context::decrypt(uint32_t _id, void * _buffer, size_t _size) noexcept
{
	return false;
}

bool no_encryption_context::does_something() const noexcept
{
	return false;
}

size_t no_encryption_context::required_output_size(size_t _input) const noexcept
{
	return _input;
}

}