#pragma once

#include "encryption_context.hpp"


namespace ysqlite3
{

class no_encryption_context : public encryption_context
{
public:
	virtual void load_app_data(const_data_t _data) override
	{
	}
	virtual void store_app_data(data_t _data) override
	{
	}
	virtual void apply_key() override
	{
	}
	virtual void revert_key() override
	{
	}
	virtual void set_key(const key_t & _key, bool _encrypt) override
	{
	}
	virtual void set_alogrithm(const char * _algorithm, bool _encrypt) override
	{
	}
	virtual bool encrypt(id_t _id, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data) override
	{
		return false;
	}
	virtual bool decrypt(id_t _id, const_buffer_t _input, size_t _size, buffer_t _output, const_data_t _data) override
	{
		return false;
	}
	virtual bool does_something() const override
	{
		return false;
	}
};

}