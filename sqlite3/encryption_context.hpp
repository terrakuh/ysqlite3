#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <type_traits>

#include "config.hpp"
#include "key.hpp"


namespace ysqlite3
{

class encryption_context
{
public:
	typedef uint8_t* data_t;
	typedef const uint8_t* const_data_t;
	typedef uint32_t id_t;
	typedef uint8_t* buffer_t;
	typedef const uint8_t* const_buffer_t;
	typedef int size_t;

	constexpr static auto page_data_size = SQLITE3_MAX_USER_DATA_SIZE;
	constexpr static auto app_data_size = 16;
	constexpr static auto header_size = 100;

	encryption_context() noexcept
	{
		_newly_created = false;
	}
	virtual ~encryption_context() = default;
	void set_newly_created(bool _value) noexcept
	{
		_newly_created = _value;
	}
	virtual void load_app_data(const_data_t _data) = 0;
	virtual void store_app_data(data_t _data) = 0;
	virtual void apply_key() = 0;
	virtual void revert_key() = 0;
	virtual void set_key(const key_t & _key, bool _encrypt) = 0;
	virtual void set_alogrithm(const char * _algorithm, bool _encrypt) = 0;
	virtual bool encrypt(id_t _id, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data) = 0;
	virtual bool decrypt(id_t _id, const_buffer_t _input, size_t _size, buffer_t _output, const_data_t _data) = 0;
	virtual bool does_something() const = 0;
	bool newly_created() const noexcept
	{
		return _newly_created;
	}

protected:
	template<typename Type>
	static typename std::enable_if<std::is_arithmetic<Type>::value>::type write(void * _buffer, Type _value)
	{
		if (little_endian()) {
			auto _ptr = reinterpret_cast<uint8_t*>(_buffer);

			for (auto i = 0; i < sizeof(Type); ++i) {
				*_ptr++ = _value >> 8 * i & 0xff;
			}

			return;
		}

		*reinterpret_cast<Type*>(_buffer) = _value;
	}
	static bool little_endian() noexcept
	{
		int _test = 1;

		return *reinterpret_cast<char*>(&_test);
	}
	template<typename Type>
	static typename std::enable_if<std::is_arithmetic<Type>::value, Type>::type read(const void * _buffer)
	{
		if (little_endian()) {
			Type _value = Type();
			auto _ptr = reinterpret_cast<const uint8_t*>(_buffer);

			for (auto i = 0; i < sizeof(Type); ++i) {
				_value = static_cast<Type>(_value | static_cast<Type>(_ptr[i]) << 8 * (sizeof(Type) - i - 1));
			}

			return _value;
		}

		return *reinterpret_cast<const Type*>(_buffer);
	}

private:
	bool _newly_created;
};

}
