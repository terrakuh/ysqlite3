#pragma once

#include <memory>

#include "config.hpp"
#include "encryption_mode.hpp"


namespace ysqlite3
{

class chacha20_poly1305_mode : public encryption_mode
{
public:
	SQLITE3_API chacha20_poly1305_mode(bool _authenticate);
	SQLITE3_API virtual ~chacha20_poly1305_mode();
	SQLITE3_API virtual bool encrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data, size_t _max_data_size) override;
	SQLITE3_API virtual bool decrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, const_data_t _data, size_t _max_data_size) override;
	SQLITE3_API virtual int iv_length() const noexcept override;
	SQLITE3_API virtual int block_size() const noexcept override;
	SQLITE3_API virtual int key_size() const noexcept override;

private:
	struct impl;

	std::unique_ptr<impl> _impl;

	bool encrypt_decrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data, bool _encrypt);
};

}