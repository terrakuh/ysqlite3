#include "aes_ccm_mode.hpp"
#include "scope_exit.hpp"

#include <openssl/evp.h>


namespace ysqlite3
{

struct aes_ccm_mode::impl
{
	EVP_CIPHER_CTX * context;
};


aes_ccm_mode::aes_ccm_mode() : _impl(new impl())
{
	_impl->context = EVP_CIPHER_CTX_new();
}

aes_ccm_mode::~aes_ccm_mode()
{
	EVP_CIPHER_CTX_free(_impl->context);
}

bool aes_ccm_mode::encrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data, size_t _max_data_size)
{
	return encrypt_decrypt(_key, _iv, _aad, _aad_size, _input, _size, _output, _data, true);
}

bool aes_ccm_mode::decrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, const_data_t _data, size_t _max_data_size)
{
	return encrypt_decrypt(_key, _iv, _aad, _aad_size, _input, _size, _output, const_cast<data_t>(_data), false);
}

int aes_ccm_mode::iv_length() const noexcept
{
	return 12;
}

int aes_ccm_mode::block_size() const noexcept
{
	return 1;
}

int aes_ccm_mode::key_size() const noexcept
{
	return 32;
}

bool aes_ccm_mode::encrypt_decrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data, bool _encrypt)
{
	if (EVP_CipherInit(_impl->context, EVP_aes_256_ccm(), nullptr, nullptr, _encrypt) == 1) {
		scope_exit _context_cleaner([this]() { EVP_CIPHER_CTX_reset(_impl->context); });
		int _len = 0;

		// Set IV length
		if (EVP_CIPHER_CTX_ctrl(_impl->context, EVP_CTRL_CCM_SET_IVLEN, 12, nullptr) != 1) {
			return false;
		}

		// Set tag length
		if (_encrypt) {
			EVP_CIPHER_CTX_ctrl(_impl->context, EVP_CTRL_CCM_SET_TAG, 16, nullptr);
		} // Set authentication tag
		else {
			if (EVP_CIPHER_CTX_ctrl(_impl->context, EVP_CTRL_CCM_SET_TAG, 16, _data) != 1) {
				return false;
			}
		}

		// Set key and IV
		if (EVP_CipherInit(_impl->context, nullptr, _key, _iv, _encrypt) != 1) {
			return false;
		}

		// Provide message length
		if (EVP_CipherUpdate(_impl->context, nullptr, &_len, nullptr, _size) != 1) {
			return false;
		}

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

		// Finalize
		if (EVP_CipherFinal(_impl->context, _data + 16, &_len) != 1) {
			return false;
		}

		// Get authentication tag
		if (_encrypt) {
			if (EVP_CIPHER_CTX_ctrl(_impl->context, EVP_CTRL_CCM_GET_TAG, 16, _data) != 1) {
				return false;
			}
		}

		return true;
	}

	return false;
}

}