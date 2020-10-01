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
#include <openssl/evp.h>
#include <openssl/rand.h>

namespace ysqlite3 {
namespace vfs {

#if YSQLITE3_ENCRYPTION_BACKEND_OPENSSL
constexpr std::uint8_t crypt_file_reserve_size() noexcept
{
	return 32;
}

template<typename File>
class crypt_file : public page_transforming_file<File>
{
public:
	using page_transforming_file<File>::page_transforming_file;
	~crypt_file()
	{
		EVP_CIPHER_CTX_free(_context);
	}
	void file_control(file_cntl operation, void* arg) override
	{
		if (operation == file_cntl::pragma) {
			auto& name       = static_cast<char**>(arg)[1];
			const auto value = static_cast<char**>(arg)[2];

			if (!std::strcmp(name, "plain_key")) {
				auto& key = _transform ? _transformation.key : _current.key;
				if (!PKCS5_PBKDF2_HMAC(value, -1, nullptr, 0, 200000, EVP_sha512(),
				                       static_cast<int>(key.size()), key.data())) {
					name = sqlite3_mprintf("%s", "failed to generate new key");
				} else {
					name = sqlite3_mprintf("%s", "ok");
				}
			} else if (!std::strcmp(name, "crypt_transformation")) {
				if ((value[0] == '0' || value[0] == '1') && !value[1]) {
					if (_transform != (value[0] == '1')) {
						(_transform ? _current : _transformation) = (_transform ? _transformation : _current);
						_transform                                = !_transform;
					}
					name = sqlite3_mprintf("%s", "ok");
				} else {
					name = sqlite3_mprintf("%s", "bad argument");
				}
			} else if (!std::strcmp(name, "cipher")) {
				auto& cipher = _transform ? _transformation.cipher : _current.cipher;
				if (!std::strcmp(value, "null")) {
					cipher = nullptr;
				} else if (!std::strcmp(value, "aes-256-gcm")) {
					cipher = EVP_aes_256_gcm();
				} else if (!std::strcmp(value, "aes-192-gcm")) {
					cipher = EVP_aes_192_gcm();
				} else if (!std::strcmp(value, "aes-128-gcm")) {
					cipher = EVP_aes_128_gcm();
				} else {
					name = sqlite3_mprintf("%s", "unkown cipher");
					return;
				}
				name = sqlite3_mprintf("%s", "ok");
			} else {
				goto gt_parent;
			}
		}

	gt_parent:
		page_transforming_file<File>::file_control(operation, arg);
	}

protected:
	void encode_page(span<std::uint8_t*> page) override
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
	void decode_page(span<std::uint8_t*> page) override
	{
		if (_current.cipher) {
			auto data = page.subspan(page.size() - crypt_file_reserve_size());
			page      = page.subspan(0, page.size() - crypt_file_reserve_size());
			_crypt(page, data, _current, false);
		}
	}
	bool check_reserve_size(std::uint8_t size) const noexcept override
	{
		return size == crypt_file_reserve_size();
	}

private:
	struct encryptor
	{
		const EVP_CIPHER* cipher = nullptr;
		std::array<unsigned char, 32> key{};
	};

	EVP_CIPHER_CTX* _context = EVP_CIPHER_CTX_new();
	bool _transform          = false;
	encryptor _current;
	encryptor _transformation;

	void _crypt(span<std::uint8_t*> buffer, span<std::uint8_t*> data, encryptor& encryptor, bool encrypt)
	{
		EVP_CIPHER_CTX_reset(_context);

		if (!EVP_CipherInit_ex(_context, encryptor.cipher, nullptr, encryptor.key.data(), data.begin(),
		                       encrypt)) {
			throw std::system_error{ sqlite3_errc::generic, "failed to encrypt/decrypt" };
		}

		int len = 0;
		if (!EVP_CipherUpdate(_context, buffer.begin(), &len, buffer.begin(),
		                      static_cast<int>(buffer.size())) ||
		    len != static_cast<int>(buffer.size())) {
			throw std::system_error{ sqlite3_errc::generic, "failed to encrypt/decrypt" };
		}

		// set tag
		if (!encrypt) {
			EVP_CIPHER_CTX_ctrl(_context, EVP_CTRL_GCM_SET_TAG, 16, data.begin() + 12);
		}

		if (!EVP_CipherFinal_ex(_context, buffer.begin(), &len) || len) {
			throw std::system_error{ sqlite3_errc::generic, "failed to encrypt/decrypt" };
		}
	}
	void _generate_iv(span<std::uint8_t*> iv) const noexcept
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
};
#endif

} // namespace vfs
} // namespace ysqlite3

#endif
