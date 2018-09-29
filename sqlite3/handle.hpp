#pragma once

#include <functional>


namespace sqlite3
{

class handle
{
public:
	handle(std::function<void(void*)> && _destructor) noexcept;
	handle(const std::function<void(void*)> & _destructor) noexcept;
	~handle();
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