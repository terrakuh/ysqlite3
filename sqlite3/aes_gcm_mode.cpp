#include "aes_gcm_mode.hpp"
#include "scope_exit.hpp"

#include <openssl/evp.h>


namespace ysqlite3
{

struct aes_gcm_mode::impl
{
	EVP_CIPHER_CTX * context;
};


aes_gcm_mode::aes_gcm_mode() : _impl(new impl())
{
	_impl->context = EVP_CIPHER_CTX_new();
}

aes_gcm_mode::~aes_gcm_mode()
{
	EVP_CIPHER_CTX_free(_impl->context);
}

bool aes_gcm_mode::encrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data, size_t _max_data_size)
{
	return encrypt_decrypt(_key, _iv, _aad, _aad_size, _input, _size, _output, _data, true);
}

bool aes_gcm_mode::decrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, const_data_t _data, size_t _max_data_size)
{
	return encrypt_decrypt(_key, _iv, _aad, _aad_size, _input, _size, _output, const_cast<data_t>(_data), false);
}

int aes_gcm_mode::iv_length() const noexcept
{
	return 12;
}

int aes_gcm_mode::block_size() const noexcept
{
	return 1;
}
inline void print(const char * _msg, const uint8_t * _ptr, int _size)
{
	printf("%s", _msg);

	while (_size-- > 0) {
		printf("%02x ", (int)*_ptr++);
	}

	puts("");
}
bool aes_gcm_mode::encrypt_decrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data, bool _encrypt)
{
	if (EVP_CipherInit(_impl->context, EVP_aes_256_gcm(), _key, _iv, _encrypt) == 1) {
		scope_exit _context_cleaner([this]() { EVP_CIPHER_CTX_reset(_impl->context); });
		int _len = 0;

		// Provide AAD data
		if (_aad && _aad_size) {
			if (EVP_CipherUpdate(_impl->context, nullptr, &_len, _aad, _aad_size) != 1) {
				return false;
			}
		}

		// Encrypt message
		if (EVP_CipherUpdate(_impl->context, _output, &_len, _input, _size) != 1) {
			return false;
		}

		// Set authentication tag
		if (!_encrypt) {
			print("set tag: ", _data, 16);
			if (EVP_CIPHER_CTX_ctrl(_impl->context, EVP_CTRL_GCM_SET_TAG, 16, _data) != 1) {
				return false;
			}
		}

		// Finalize
		if (EVP_CipherFinal(_impl->context, _data + 16, &_len) != 1) {
			return false;
		}

		// Get authentication tag
		if (_encrypt) {
			if (EVP_CIPHER_CTX_ctrl(_impl->context, EVP_CTRL_GCM_GET_TAG, 16, _data) != 1) {
				return false;
			}
			print("get tag: ", _data, 16);
		}

		return true;
	}

	return false;
}

}