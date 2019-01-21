#pragma once

#include <cstdint>
#include <array>

#include "config.hpp"


namespace ysqlite3
{

class encryption_context
{
public:
	typedef int8_t* data_t;
	typedef const int8_t* const_data_t;
	typedef uint32_t id_t;
	typedef void* buffer_t;
	typedef const void* const_buffer_t;
	typedef int size_t;

	constexpr static auto max_page_data_size = SQLITE3_MAX_USER_DATA_SIZE;
	constexpr static auto max_app_data_size = 16;

	virtual ~encryption_context() = default;
	virtual void load_app_data(const_data_t _data) = 0;
	virtual void store_app_data(data_t _data) = 0;
	virtual bool encrypt(id_t _id, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data) = 0;
	virtual bool decrypt(id_t _id, const_buffer_t _input, size_t _size, buffer_t _output, const_data_t _data) = 0;
	virtual bool encrypts() const = 0;
	virtual bool decrypts() const = 0;
};

}
