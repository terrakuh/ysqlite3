#include "handle.hpp"

#include <utility>


namespace ysqlite3
{

handle::handle(std::function<void(void*)> && _destructor) noexcept : _destructor(std::move(_destructor))
{
	_handle = nullptr;
}

handle::handle(const std::function<void(void*)> & _destructor) noexcept : _destructor(_destructor)
{
	_handle = nullptr;
}

handle::~handle()
{
	if (_handle) {
		_destructor(_handle);
	}
}

}