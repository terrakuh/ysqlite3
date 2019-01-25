#include "openssl_encryption_context.hpp"
#include "scope_exit.hpp"
#include "aes_gcm_mode.hpp"

#include <cstring>
#include <openssl/rand.h>
#include <openssl/err.h>


namespace ysqlite3
{

openssl_encryption_context::openssl_encryption_context() : _encryptor(new aes_gcm_mode()), _decryptor(_encryptor)
{
	_kdf_iterations = 0;
	_context = EVP_CIPHER_CTX_new();
	_md_context = EVP_MD_CTX_new();
	//_encryption_cipher = EVP_aes_256_gcm();
	//_decryption_cipher = EVP_aes_256_gcm();

}

openssl_encryption_context::~openssl_encryption_context()
{
	EVP_MD_CTX_free(_md_context);
	EVP_CIPHER_CTX_free(_context);
}

void openssl_encryption_context::load_app_data(const_data_t _data)
{
	if (_kdf_iterations) {
		return;
	}

	if (_data) {
		//_kdf_iterations = read<int32_t>(_data);

		std::memcpy(_salt.data(), _data, app_data_size);
	} else {
		random_bytes(_salt.data(), _salt.size());
	}

	print(_data ? "loaded salt: " : "generated salt: ", _salt.data(), _salt.size());
}

void openssl_encryption_context::store_app_data(data_t _data)
{
	print("save salt: ", _salt.data(), _salt.size());
	std::memcpy(_data, _salt.data(), _salt.size());
}

void openssl_encryption_context::apply_key()
{
	std::memcpy(_encryption_key2.data(), _encryption_key.data(), _encryption_key.size());
	std::memcpy(_decryption_key2.data(), _decryption_key.data(), _decryption_key.size());
}

void openssl_encryption_context::revert_key()
{
	std::memcpy(_encryption_key.data(), _encryption_key2.data(), _encryption_key2.size());
	std::memcpy(_decryption_key.data(), _decryption_key2.data(), _decryption_key2.size());
}

void openssl_encryption_context::set_key(const key_t & _key, bool _encrypt)
{
	// Set default
	if (!_kdf_iterations) {
		_kdf_iterations = 500000;
	}

	auto & _destination = _encrypt ? _encryption_key : _decryption_key;

	_destination.swap(_encrypt ? _encryption_key2 : _decryption_key2);

	PKCS5_PBKDF2_HMAC(_key.data(), _key.length(), _salt.data(), _salt.size(), _kdf_iterations, EVP_sha512(), _destination.size(), _destination.data());

	print(_encrypt ? "encryption key: " : "decryption key: ", _destination.data(), _destination.size());
}

void openssl_encryption_context::set_alogrithm(const char * _algorithm, bool _encrypt)
{
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
	auto _block_size = _encryptor->block_size();
	auto _max_data_size = page_data_size;

	if (auto _tmp = _size % _block_size) {
		_size += _tmp;
		_data += _tmp;
		_max_data_size -= _tmp;
	}

	// Generate new IV
	auto _iv_length = _encryptor->iv_length();

	random_bytes(_data, _iv_length);

	print("generated iv: ", _data, _iv_length);

	// Create page key
	internal_key_t _page_key;

	if (!create_page_key(_id, _encryption_key, _page_key)) {
		return false;
	}

	print("page key: ", _page_key.data(), _page_key.size());

	scope_exit _key_cleaner([&_page_key, _output]() { std::memset(_page_key.data(), 0, _page_key.size());
	print(">>>>", _output, 16); });

	return _encryptor->encrypt(_page_key.data(), _data, _aad, _aad_size, _input, _size, _output, _data + _iv_length, _max_data_size - _iv_length);
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
	auto _block_size = _decryptor->block_size();
	auto _max_data_size = page_data_size;

	if (auto _tmp = _size % _block_size) {
		_size += _tmp;
		_data += _tmp;
		_max_data_size -= _tmp;
	}

	// Get IV length
	auto _iv_length = _decryptor->iv_length();

	print("read iv: ", _data, _iv_length);

	// Create page key
	internal_key_t _page_key;

	if (!create_page_key(_id, _decryption_key, _page_key)) {
		return false;
	}

	print("page key: ", _page_key.data(), _page_key.size());

	scope_exit _key_cleaner([&_page_key]() { std::memset(_page_key.data(), 0, _page_key.size()); });
	print("<<<<", _input, 16);
	return _decryptor->decrypt(_page_key.data(), _data, _aad, _aad_size, _input, _size, _output, _data + _iv_length, _max_data_size - _iv_length);
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
	std::memcpy(_page_key.data(), _key.data(), _key.size());

	return true;

	if (EVP_DigestInit(_md_context, EVP_sha512()) == 1) {
		scope_exit _exit([this]() { EVP_MD_CTX_reset(_md_context); });

		if (EVP_DigestUpdate(_md_context, &_id, sizeof(_id)) != 1) {
			return false;
		}

		if (EVP_DigestUpdate(_md_context, _salt.data(), _salt.size()) != 1) {
			return false;
		}

		if (EVP_DigestUpdate(_md_context, _key.data(), _key.size()) != 1) {
			return false;
		}

		std::array<unsigned char, EVP_MAX_MD_SIZE> _output;
		scope_exit _cleaner([&_output]() { std::memset(_output.data(), 0, _output.size()); });

		if (EVP_DigestFinal(_md_context, _output.data(), nullptr) != 1) {
			return false;
		}

		std::memcpy(_page_key.data(), _output.data(), _page_key.size());

		return true;
	}

	return false;
}

}