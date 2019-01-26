#pragma once

#include <memory>

#include "encryption_mode.hpp"


namespace ysqlite3
{

class chacha20_poly1305_mode : public encryption_mode
{
public:
	chacha20_poly1305_mode(bool _authenticate);
	virtual ~chacha20_poly1305_mode();
	virtual bool encrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data, size_t _max_data_size) override;
	virtual bool decrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, const_data_t _data, size_t _max_data_size) override;
	virtual int iv_length() const noexcept override;
	virtual int block_size() const noexcept override;
	virtual int key_size() const noexcept override;

private:
	struct impl;

	std::unique_ptr<impl> _impl;

	bool encrypt_decrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data, bool _encrypt);
};

}