#pragma once

#include "encryption_context.hpp"


namespace ysqlite3
{

class encryption_mode
{
public:
	typedef const uint8_t* const_key_t;
	typedef encryption_context::buffer_t buffer_t;
	typedef encryption_context::const_buffer_t const_buffer_t;
	typedef encryption_context::size_t size_t;
	typedef encryption_context::data_t data_t;
	typedef encryption_context::const_data_t const_data_t;

	virtual ~encryption_mode() noexcept = default;
	virtual bool encrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data, size_t _max_data_size) = 0;
	virtual bool decrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, const_data_t _data, size_t _max_data_size) = 0;
	virtual int iv_length() const noexcept = 0;
	virtual int block_size() const noexcept = 0;
	virtual int key_size() const noexcept = 0;
};

}