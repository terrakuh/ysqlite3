#include "encryption_context.hpp"


namespace sqlite3
{

#include "sqlite3.h"


void encryption_context::set_read_callback(read_callback_signature && _callback) noexcept
{
	_read_callback.swap(_callback);
}

void encryption_context::set_read_callback(const read_callback_signature & _callback) noexcept
{
	_read_callback = _callback;
}

void encryption_context::set_sector_size(size_t _size) noexcept
{
	_sector_size = _size;
}

int encryption_context::sync() noexcept
{
	return SQLITE_OK;
}

int64_t encryption_context::last_encryption_offset() const noexcept
{
	return _last_encryption_offset;
}

bool encryption_context::can_read() const noexcept
{
	return static_cast<bool>(_read_callback);
}

int encryption_context::read(void * _output, int _size, int64_t _offset) noexcept
{
	return _read_callback(_output, _size, _offset);
}

}