#ifndef YSQLITE3_VFS_CRYPT_FILE_HPP_
#define YSQLITE3_VFS_CRYPT_FILE_HPP_

#include "../config.hpp"
#include "../error.hpp"
#include "page_transforming_file.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>

#if YSQLITE3_ENCRYPTION_BACKEND_OPENSSL
#	include <openssl/evp.h>
#	include <openssl/rand.h>
#endif

namespace ysqlite3 {
namespace vfs {

#if YSQLITE3_ENCRYPTION_BACKEND_OPENSSL
constexpr std::uint8_t crypt_file_reserve_size() noexcept
{
	return 32;
}

template<typename Parent>
class Crypt_file : public Page_transforming_file<Parent>
{
public:
	template<typename... Args>
	Crypt_file(Args&&... args) : Page_transforming_file<Parent>{ std::forward<Args>(args)... }
	{
		if (const auto key = sqlite3_uri_parameter(this->name, "key")) {
			sqlite3_free(_generate_key(key, _current));
		}

		if (const auto cipher = sqlite3_uri_parameter(this->name, "cipher")) {
			sqlite3_free(_set_chiper(cipher, _current));
		}
	}
	~Crypt_file()
	{
		EVP_CIPHER_CTX_free(_context);
		std::memset(_current.key.data(), 0, _current.key.size());
		std::memset(_transformation.key.data(), 0, _transformation.key.size());
	}
	void file_control(File_control operation, void* arg) override
	{
		if (operation == File_control::pragma) {
			auto& name       = static_cast<char**>(arg)[1];
			const auto value = static_cast<char**>(arg)[2];

			if (!std::strcmp(name, "key")) {
				name = _generate_key(value, _transform ? _transformation : _current);
			} else if (!std::strcmp(name, "crypt_transformation")) {
				if ((value[0] == '0' || value[0] == '1') && !value[1]) {
					if (_transform != (value[0] == '1')) {
						(_transform ? _current : _transformation) = (_transform ? _transformation : _current);
						_transform                                = !_transform;
					}
					name = sqlite3_mprintf("ok");
				} else {
					name = sqlite3_mprintf("bad argument");
				}
			} else if (!std::strcmp(name, "cipher")) {
				name = _set_chiper(value, _transform ? _transformation : _current);
			} else {
				goto gt_parent;
			}
			return;
		}

	gt_parent:
		Page_transforming_file<Parent>::file_control(operation, arg);
	}

protected:
	void encode_page(Span<std::uint8_t*> page) override
	{
		auto& enc = _transform ? _transformation : _current;
		if (enc.cipher) {
			auto data = page.subspan(page.size() - crypt_file_reserve_size());
			page      = page.subspan(0, page.size() - crypt_file_reserve_size());
			_generate_iv(data.subspan(0, 12));
			_crypt(page, data, enc, true);
			EVP_CIPHER_CTX_ctrl(_context, EVP_CTRL_GCM_GET_TAG, 16, data.begin() + 12);
		}
	}
	void decode_page(Span<std::uint8_t*> page) override
	{
		if (_current.cipher) {
			auto data = page.subspan(page.size() - crypt_file_reserve_size());
			page      = page.subspan(0, page.size() - crypt_file_reserve_size());
			_crypt(page, data, _current, false);
		}
	}
	bool check_reserve_size(std::uint8_t size) const noexcept override
	{
		return !(_transform ? _transformation : _current).cipher || size == crypt_file_reserve_size();
	}

private:
	struct Encryptor
	{
		const EVP_CIPHER* cipher = nullptr;
		std::array<unsigned char, 32> key{};
	};

	EVP_CIPHER_CTX* _context = EVP_CIPHER_CTX_new();
	bool _transform          = false;
	Encryptor _current;
	Encryptor _transformation;

	void _crypt(Span<std::uint8_t*> buffer, Span<std::uint8_t*> data, Encryptor& encryptor, bool encrypt)
	{
		EVP_CIPHER_CTX_reset(_context);
		if (!EVP_CipherInit_ex(_context, encryptor.cipher, nullptr, encryptor.key.data(), data.begin(),
		                       encrypt)) {
			throw std::system_error{ SQLite3_code::generic, "failed to encrypt/decrypt" };
		}

		int len = 0;
		if (!EVP_CipherUpdate(_context, buffer.begin(), &len, buffer.begin(),
		                      static_cast<int>(buffer.size())) ||
		    len != static_cast<int>(buffer.size())) {
			throw std::system_error{ SQLite3_code::generic, "failed to encrypt/decrypt" };
		}

		// set tag
		if (!encrypt) {
			EVP_CIPHER_CTX_ctrl(_context, EVP_CTRL_GCM_SET_TAG, 16, data.begin() + 12);
		}

		if (!EVP_CipherFinal_ex(_context, buffer.begin(), &len) || len) {
			throw std::system_error{ SQLite3_code::not_a_database, "failed to encrypt/decrypt" };
		}
	}
	void _generate_iv(Span<std::uint8_t*> iv) const noexcept
	{
		RAND_bytes(reinterpret_cast<unsigned char*>(iv.begin()), static_cast<int>(iv.size() - 4));
#	if YSQLITE3_BIG_ENDIAN
		std::reverse(iv.end() - 4, iv.end());
		*reinterpret_cast<std::uint32_t*>(iv.end() - 4) += 1;
		std::reverse(iv.end() - 4, iv.end());
#	else
		*reinterpret_cast<std::uint32_t*>(iv.end() - 4) += 1;
#	endif
	}
	char* _generate_key(const char* value, Encryptor& encryptor)
	{
		std::string key;
		std::size_t length = std::char_traits<char>::length(value);

		// check for ' or "
		if (length < 3 || value[1] != value[length - 1] || (value[1] != '\'' && value[1] != '"')) {
			return sqlite3_mprintf("bad key format");
		}
		length -= 3;

		// raw key
		if (value[0] == 'r') {
			key.append(value + 2, length);
		} // hex key
		else if (value[0] == 'x') {
			const auto convert = [](char c) {
				if (c >= '0' && c <= '9') {
					return c - '0';
				} else if (c >= 'a' && c <= 'f') {
					return c - 'a' + 10;
				} else if (c >= 'A' && c <= 'F') {
					return c - 'A' + 10;
				}
				return -1;
			};
			std::size_t i = length % 2;
			value += 2;
			key.resize(length / 2 + i);

			if (i) {
				const int v = convert(*value);
				if (v < 0) {
					return sqlite3_mprintf("bad hex character: '%c'", *value);
				}
				key[0] = static_cast<char>(v);
				--value;
			}

			for (; i < key.size(); ++i) {
				for (int j = 0; j < 2; ++j) {
					const char c = value[i * 2 + j];
					const int v  = convert(c);
					if (v < 0) {
						return sqlite3_mprintf("bad hex character: '%c'", c);
					}
					key[i] = static_cast<char>(key[i] << 4 | v);
				}
			}
		} else {
			return sqlite3_mprintf("unknown format");
		}

		if (!PKCS5_PBKDF2_HMAC(key.c_str(), static_cast<int>(key.size()), nullptr, 0, 200000, EVP_sha512(),
		                       static_cast<int>(encryptor.key.size()), encryptor.key.data())) {
			return sqlite3_mprintf("failed to generate new key");
		}
		return sqlite3_mprintf("ok");
	}
	char* _set_chiper(const char* value, Encryptor& encryptor)
	{
		if (!std::strcmp(value, "null")) {
			encryptor.cipher = nullptr;
		} else if (!std::strcmp(value, "aes-256-gcm")) {
			encryptor.cipher = EVP_aes_256_gcm();
		} else if (!std::strcmp(value, "aes-192-gcm")) {
			encryptor.cipher = EVP_aes_192_gcm();
		} else if (!std::strcmp(value, "aes-128-gcm")) {
			encryptor.cipher = EVP_aes_128_gcm();
		} else {
			return sqlite3_mprintf("unkown cipher");
		}
		return sqlite3_mprintf("ok");
	}
};
#endif

} // namespace vfs
} // namespace ysqlite3

#endif
