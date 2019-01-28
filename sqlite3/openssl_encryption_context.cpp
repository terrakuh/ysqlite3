#include "openssl_encryption_context.hpp"
#include "scope_exit.hpp"
#include "aes_gcm_mode.hpp"
#include "aes_ccm_mode.hpp"
#include "chacha20_poly1305_mode.hpp"
#include "chacha20_aes_gcm_mode.hpp"

#include <cstring>
#include <cctype>
#include <openssl/rand.h>
#include <openssl/evp.h>


namespace ysqlite3
{

struct openssl_encryption_context::impl
{
	int32_t kdf_iterations;
	EVP_MD_CTX * md_context;
	std::array<unsigned char, app_data_size> salt;
	internal_key_t encryption_key;
	internal_key_t encryption_key2;
	internal_key_t decryption_key;
	internal_key_t decryption_key2;
	std::shared_ptr<encryption_mode> encryptor;
	std::shared_ptr<encryption_mode> decryptor;
};

openssl_encryption_context::openssl_encryption_context() : _impl(new impl())
{
	_impl->kdf_iterations = 0;
	_impl->md_context = EVP_MD_CTX_new();

}

openssl_encryption_context::~openssl_encryption_context()
{
	EVP_MD_CTX_free(_impl->md_context);
}

void openssl_encryption_context::load_app_data(const_data_t _data)
{
	if (_impl->kdf_iterations) {
		return;
	}

	if (_data) {
		//_kdf_iterations = read<int32_t>(_data);

		std::memcpy(_impl->salt.data(), _data, app_data_size);
	} else {
		random_bytes(_impl->salt.data(), _impl->salt.size());
	}
}

void openssl_encryption_context::store_app_data(data_t _data)
{
	std::memcpy(_data, _impl->salt.data(), _impl->salt.size());
}

void openssl_encryption_context::apply_key()
{
	std::memcpy(_impl->encryption_key2.data(), _impl->encryption_key.data(), _impl->encryption_key.size());
	std::memcpy(_impl->decryption_key2.data(), _impl->decryption_key.data(), _impl->decryption_key.size());
}

void openssl_encryption_context::revert_key()
{
	std::memcpy(_impl->encryption_key.data(), _impl->encryption_key2.data(), _impl->encryption_key2.size());																		 
	std::memcpy(_impl->decryption_key.data(), _impl->decryption_key2.data(), _impl->decryption_key2.size());
}

void openssl_encryption_context::set_key(const key_t & _key, bool _encrypt)
{
	// Set default
	if (!_impl->kdf_iterations) {
		_impl->kdf_iterations = 500000;
	}

	auto & _destination = _encrypt ? _impl->encryption_key : _impl->decryption_key;

	_destination.swap(_encrypt ? _impl->encryption_key2 : _impl->decryption_key2);

	PKCS5_PBKDF2_HMAC(_key.data(), _key.length(), _impl->salt.data(), _impl->salt.size(), _impl->kdf_iterations, EVP_sha512(), _destination.size(), _destination.data());
}

void openssl_encryption_context::set_alogrithm(const char * _algorithm, bool _encrypt)
{
	auto & _cryptor = _encrypt ? _impl->encryptor : _impl->decryptor;

	if (iequals(_algorithm, "aes-256-gcm")) {
		_cryptor.reset(new aes_gcm_mode());
	} else if (iequals(_algorithm, "aes-256-ccm")) {
		_cryptor.reset(new aes_ccm_mode());
	} else if (iequals(_algorithm, "chacha20-poly1305")) {
		_cryptor.reset(new chacha20_poly1305_mode(true));
	} else if (iequals(_algorithm, "chacha20-aes-gcm")) {
		_cryptor.reset(new chacha20_aes_gcm_mode());
	}
}

bool openssl_encryption_context::encrypt(id_t _id, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data)
{
	// Set AAD
	const_buffer_t _aad = nullptr;
	size_t _aad_size = 0;

	if (!_id) {
		_aad = _output + app_data_size;
		_aad_size = header_size - app_data_size;
		_output += header_size;
	}

	// Add padding
	auto _block_size = _impl->encryptor->block_size();

	//_size += _block_size;
	_data += _block_size;

	// Generate new IV
	auto _iv_length = _impl->encryptor->iv_length();

	random_bytes(_data, _iv_length);

	// Create page key
	internal_key_t _page_key;

	if (!create_page_key(_id, _impl->encryption_key, _page_key)) {
		return false;
	}

	scope_exit _key_cleaner([&_page_key, _output]() { std::memset(_page_key.data(), 0, _page_key.size()); });

	return _impl->encryptor->encrypt(_page_key.data(), _data, _aad, _aad_size, _input, _size, _output, _data + _iv_length, page_data_size - _block_size - _iv_length);
}

bool openssl_encryption_context::decrypt(id_t _id, const_buffer_t _input, size_t _size, buffer_t _output, const_data_t _data)
{
	// Set AAD
	const_buffer_t _aad = nullptr;
	size_t _aad_size = 0;

	if (!_id) {
		_aad = _output + app_data_size;
		_aad_size = header_size - app_data_size;
		_output += header_size;
	}

	// Add padding
	auto _block_size = _impl->decryptor->block_size();

	//_size += _block_size;
	_data += _block_size;

	// Get IV length
	auto _iv_length = _impl->decryptor->iv_length();

	// Create page key
	internal_key_t _page_key;

	if (!create_page_key(_id, _impl->decryption_key, _page_key)) {
		return false;
	}

	scope_exit _key_cleaner([&_page_key]() { std::memset(_page_key.data(), 0, _page_key.size()); });

	return _impl->decryptor->decrypt(_page_key.data(), _data, _aad, _aad_size, _input, _size, _output, _data + _iv_length, page_data_size - _block_size - _iv_length);
}

bool openssl_encryption_context::does_something() const
{
	return true;
}

void openssl_encryption_context::random_bytes(void * _buffer, int _size)
{
	auto i = RAND_bytes(reinterpret_cast<unsigned char*>(_buffer), _size);
}

bool openssl_encryption_context::create_page_key(id_t _id, const internal_key_t & _key, internal_key_t & _page_key)
{
	if (EVP_DigestInit(_impl->md_context, EVP_sha512()) == 1) {
		scope_exit _exit([this]() { EVP_MD_CTX_reset(_impl->md_context); });

		if (EVP_DigestUpdate(_impl->md_context, &_id, sizeof(_id)) != 1) {
			return false;
		}

		if (EVP_DigestUpdate(_impl->md_context, _impl->salt.data(), _impl->salt.size()) != 1) {
			return false;
		}

		if (EVP_DigestUpdate(_impl->md_context, _key.data(), _key.size()) != 1) {
			return false;
		}

		std::array<unsigned char, EVP_MAX_MD_SIZE> _output;
		scope_exit _cleaner([&_output]() { std::memset(_output.data(), 0, _output.size()); });

		if (EVP_DigestFinal(_impl->md_context, _output.data(), nullptr) != 1) {
			return false;
		}

		std::memcpy(_page_key.data(), _output.data(), _page_key.size());

		return true;
	}

	return false;
}

bool openssl_encryption_context::iequals(const char * _str1, const char * _str2)
{
	char _c1, _c2;

	while (true) {
		_c1 = *_str1++, _c2 = *_str2++;

		if (!_c1 || !_c2) {
			break;
		} else if (std::tolower(_c1) != std::tolower(_c2)) {
			return false;
		}
	}

	return _c1 == _c2;
}

}