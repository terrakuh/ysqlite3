#include "scope_exit.hpp"

#include <utility>


namespace ysqlite3
{

scope_exit::scope_exit(function_t && _exit) noexcept : _exit(std::move(_exit))
{
}

scope_exit::~scope_exit()
{
	if (_exit) {
		_exit();
	}
}

void scope_exit::cancel() noexcept
{
	function_t _tmp;

	_exit.swap(_tmp);
}

}