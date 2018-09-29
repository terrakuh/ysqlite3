#pragma once


namespace sqlite3
{

class sqlite3_connection
{
public:
	sqlite3_connection();
	~sqlite3_connection();
	template<typename Type>
	Type *& handle() noexcept
	{
		return static_cast<Type*>(_handle);
	}

private:
	void * _handle;
};

}