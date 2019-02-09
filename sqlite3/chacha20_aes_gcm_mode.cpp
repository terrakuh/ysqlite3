#include "chacha20_aes_gcm_mode.hpp"
#include "chacha20_poly1305_mode.hpp"
#include "aes_gcm_mode.hpp"
#include "scope_exit.hpp"

#include <vector>


namespace ysqlite3
{

struct chacha20_aes_gcm_mode::impl
{
	chacha20_poly1305_mode chacha20;
	aes_gcm_mode aes_gcm;
	std::vector<uint8_t> tmp_buffer;

	impl() : chacha20(false)
	{
	}
};

chacha20_aes_gcm_mode::chacha20_aes_gcm_mode() : _impl(new impl())
{
}

chacha20_aes_gcm_mode::~chacha20_aes_gcm_mode()
{
}

bool chacha20_aes_gcm_mode::encrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, data_t _data, size_t _max_data_size)
{
	// Resize temp buffer
	_impl->tmp_buffer.reserve(_size);

	scope_exit _cleaner([this]() { std::memset(_impl->tmp_buffer.data(), 0, _impl->tmp_buffer.capacity()); });

	// Encrypt with ChaCha20
	if (!_impl->chacha20.encrypt(_key, _iv, nullptr, 0, _input, _size, _impl->tmp_buffer.data(), _data, _max_data_size)) {
		return false;
	}

	// Encrypt and authenticate with AES-GCM
	return _impl->aes_gcm.encrypt(_key + 32, _iv + 12, _aad, _aad_size, _impl->tmp_buffer.data(), _size, _output, _data, _max_data_size);
}

bool chacha20_aes_gcm_mode::decrypt(const_key_t _key, const_buffer_t _iv, const_buffer_t _aad, size_t _aad_size, const_buffer_t _input, size_t _size, buffer_t _output, const_data_t _data, size_t _max_data_size)
{
	// Resize temp buffer
	_impl->tmp_buffer.reserve(_size);

	scope_exit _cleaner([this]() { std::memset(_impl->tmp_buffer.data(), 0, _impl->tmp_buffer.capacity()); });

	// Decrypt with AES-GCM
	if (!_impl->aes_gcm.decrypt(_key + 32, _iv + 12, _aad, _aad_size, _input, _size, _impl->tmp_buffer.data(), _data, _max_data_size)) {
		return false;
	}

	// Decrypt with ChaCha20
	return _impl->chacha20.decrypt(_key, _iv, nullptr, 0, _impl->tmp_buffer.data(), _size, _output, _data, _max_data_size);
}

int chacha20_aes_gcm_mode::iv_length() const noexcept
{
	return 24;
}

int chacha20_aes_gcm_mode::block_size() const noexcept
{
	return 1;
}

int chacha20_aes_gcm_mode::key_size() const noexcept
{
	return 64;
}

}