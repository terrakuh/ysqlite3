#pragma once


namespace sqlite3
{

class sqlite3_dataholder
{
public:
	sqlite3_dataholder();
	~sqlite3_dataholder();
	template<typename Type>
	Type *& handle() noexcept
	{
		return static_cast<Type*>(_handle);
	}

private:
	void * _handle;
};

}