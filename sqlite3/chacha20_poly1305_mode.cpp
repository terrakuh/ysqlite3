#include "chacha20_poly1305_mode.hpp"
#include "scope_exit.hpp"

#include <openssl/evp.h>


namespace ysqlite3
{

struct chacha20_poly1305_mode::impl
{
	bool authenticate;
	EVP_CIPHER_CTX * context;
};


chacha20_poly1305_mode::chacha20_poly1305_mode(bool _authenticate) : _impl(new impl())
{
	_impl->authenticate = _authenticate;
	_impl->context = EVP_CIPHER_CTX_new();
}

chacha20_poly1305_mode::~chacha20_poly1305_mode()
{
	EVP_CIPHER_CTX_free(_impl->context);
}

bool chacha20_poly1305_mode::encrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data, size_t _max_data_size)
{
	return encrypt_decrypt(_key, _iv, _aad, _aad_size, _input, _size, _output, _data, true);
}

bool chacha20_poly1305_mode::decrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, const_data_t _data, size_t _max_data_size)
{
	return encrypt_decrypt(_key, _iv, _aad, _aad_size, _input, _size, _output, const_cast<data_t>(_data), false);
}

int chacha20_poly1305_mode::iv_length() const noexcept
{
	return 12;
}

int chacha20_poly1305_mode::block_size() const noexcept
{
	return 1;
}

bool chacha20_poly1305_mode::encrypt_decrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data, bool _encrypt)
{
	if (EVP_CipherInit(_impl->context, _impl->authenticate ? EVP_chacha20_poly1305() : EVP_chacha20(), _key, _iv, _encrypt) == 1) {
		scope_exit _context_cleaner([this]() { EVP_CIPHER_CTX_reset(_impl->context); });
		int _len = 0;

		// Provide AAD data
		if (_impl->authenticate && _aad && _aad_size) {
			if (EVP_CipherUpdate(_impl->context, nullptr, &_len, _aad, _aad_size) != 1) {
				return false;
			}
		}

		// Encrypt message
		if (EVP_CipherUpdate(_impl->context, _output, &_len, _input, _size) != 1) {
			return false;
		}

		// Set authentication tag
		if (_impl->authenticate && !_encrypt) {
			if (EVP_CIPHER_CTX_ctrl(_impl->context, EVP_CTRL_GCM_SET_TAG, 16, _data) != 1) {
				return false;
			}
		}

		// Finalize
		if (EVP_CipherFinal(_impl->context, _data + 16, &_len) != 1) {
			return false;
		}

		// Get authentication tag
		if (_impl->authenticate && _encrypt) {
			if (EVP_CIPHER_CTX_ctrl(_impl->context, EVP_CTRL_GCM_GET_TAG, 16, _data) != 1) {
				return false;
			}
		}

		return true;
	}

	return false;
}

}