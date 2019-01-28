#pragma once

#include <array>
#include <memory>

#include "config.hpp"
#include "encryption_context.hpp"


namespace ysqlite3
{

class openssl_encryption_context : public encryption_context
{
public:
	SQLITE3_API openssl_encryption_context();
	SQLITE3_API ~openssl_encryption_context();
	SQLITE3_API virtual void load_app_data(const_data_t _data) override;
	SQLITE3_API virtual void store_app_data(data_t _data) override;
	SQLITE3_API virtual void apply_key() override;
	SQLITE3_API virtual void revert_key() override;
	SQLITE3_API virtual void set_key(const key_t & _key, bool _encrypt) override;
	SQLITE3_API virtual void set_alogrithm(const char * _algorithm, bool _encrypt) override;
	SQLITE3_API virtual bool encrypt(id_t _id, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data) override;
	SQLITE3_API virtual bool decrypt(id_t _id, const_buffer_t _input, size_t _size, buffer_t _output, const_data_t _data) override;
	SQLITE3_API virtual bool does_something() const override;

protected:
	typedef std::array<unsigned char, 64> internal_key_t;

	struct impl;

	std::unique_ptr<impl> _impl;

	SQLITE3_API virtual void random_bytes(void * _buffer, int _size);
	SQLITE3_API bool create_page_key(id_t _id, const internal_key_t & _key, internal_key_t & _page_key);
	SQLITE3_API static bool iequals(const char * _str1, const char * _str2);
};

}
