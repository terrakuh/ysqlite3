#pragma once

#include <cstdint>
#include <cstddef>
#include <array>


namespace ysqlite3
{

class encryption_context
{
public:
	virtual void load_data()
	{

	}
	virtual void load_data(std::array<int8_t, 16> _data)
	{

	}
	virtual bool does_something() const
	{
		return false;
	}
	virtual bool encrypt(uint32_t _id, const void * _buffer, size_t _size, void * _output)
	{
		return false;
	}
	virtual bool decrypt(uint32_t _id, const void * _buffer, size_t _size, void * _output)
	{
		return false;
	}
	virtual std::array<int8_t, 16> save_data()
	{
		return {};
	}
};

}
