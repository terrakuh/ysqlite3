#pragma once

#include <functional>

#include "config.hpp"


namespace sqlite3
{

class handle
{
public:
	SQLITE3_API handle(std::function<void(void*)> && _destructor) noexcept;
	SQLITE3_API handle(const std::function<void(void*)> & _destructor) noexcept;
	SQLITE3_API ~handle();
	template<typename Type>
	Type *& get()
	{
		return *reinterpret_cast<Type**>(&_handle);
	}

private:
	void * _handle;
	std::function<void(void*)> _destructor;
};

}