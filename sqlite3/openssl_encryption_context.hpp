#pragma once

#include <openssl/evp.h>
#include <vector>
#include <cstdint>
#include <array>

#include "encryption_context.hpp"
#include "encryption_mode.hpp"


namespace ysqlite3
{

class openssl_encryption_context : public encryption_context
{
public:
	openssl_encryption_context();

	~openssl_encryption_context();
	virtual void load_app_data(const_data_t _data) override;
	virtual void store_app_data(data_t _data) override;
	virtual void apply_key() override;
	virtual void revert_key() override;
	virtual void set_key(const key_t & _key, bool _encrypt) override;
	virtual void set_alogrithm(const char * _algorithm, bool _encrypt) override;
	virtual bool encrypt(id_t _id, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data) override;
	virtual bool decrypt(id_t _id, const_buffer_t _input, size_t _size, buffer_t _output, const_data_t _data) override;
	virtual bool does_something() const override;

protected:
	typedef std::array<unsigned char, EVP_MAX_KEY_LENGTH> internal_key_t;

	int32_t _kdf_iterations;
	EVP_CIPHER_CTX * _context;
	EVP_MD_CTX * _md_context;
	std::array<unsigned char, app_data_size> _salt;
	internal_key_t _encryption_key;
	internal_key_t _encryption_key2;
	internal_key_t _decryption_key;
	internal_key_t _decryption_key2;
	std::shared_ptr<encryption_mode> _encryptor;
	std::shared_ptr<encryption_mode> _decryptor;

	static void print(const char * _msg, const uint8_t * _ptr, int _size)
	{
		printf("%s", _msg);

		while (_size-- > 0) {
			printf("%02x ", (int)*_ptr++);
		}

		puts("");
	}
	virtual void random_bytes(void * _buffer, int _size);
	bool create_page_key(id_t _id, const internal_key_t & _key, internal_key_t & _page_key);
	static bool iequals(const char * _str1, const char * _str2);
};

}
