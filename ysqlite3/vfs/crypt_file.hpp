#ifndef YSQLITE3_VFS_CRYPT_FILE_HPP_
#define YSQLITE3_VFS_CRYPT_FILE_HPP_

#include "../config.hpp"
#include "../error.hpp"
#include "page_transforming_file.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
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
class crypt_file : public vfs::page_transforming_file<File>
{
public:
	using page_transforming_file<File>::page_transforming_file;
	~crypt_file()
	{
		EVP_CIPHER_CTX_free(_context);
	}
	void file_control(vfs::file_cntl operation, void* arg) override
	{
		if (operation == vfs::file_cntl::pragma) {
			auto& name       = static_cast<char**>(arg)[1];
			const auto value = static_cast<char**>(arg)[2];

			if (!std::strcmp(name, "plain_key")) {
				if (!PKCS5_PBKDF2_HMAC(value, -1, nullptr, 0, 200000, EVP_sha512(),
				                       static_cast<int>(_key.size()), _key.data())) {
					throw std::system_error{ sqlite3_errc::generic, "failed to generate new key" };
				}
				name = sqlite3_mprintf("%s", "ok");
				return;
			}
		}

		page_transforming_file<File>::file_control(operation, arg);
	}

protected:
	void encode_page(span<std::uint8_t*> page) override
	{
		auto data = page.subspan(page.size() - crypt_file_reserve_size());
		page      = page.subspan(0, page.size() - crypt_file_reserve_size());
		_generate_iv(data.subspan(0, 12));
		_crypt(page, data, true);
		EVP_CIPHER_CTX_ctrl(_context, EVP_CTRL_GCM_GET_TAG, 16, data.begin() + 12);
	}
	void decode_page(span<std::uint8_t*> page) override
	{
		auto data = page.subspan(page.size() - crypt_file_reserve_size());
		page      = page.subspan(0, page.size() - crypt_file_reserve_size());
		_crypt(page, data, false);
	}
	bool check_reserve_size(std::uint8_t size) const noexcept override
	{
		return size == crypt_file_reserve_size();
	}

private:
	EVP_CIPHER_CTX* _context  = EVP_CIPHER_CTX_new();
	const EVP_CIPHER* _cipher = EVP_aes_256_gcm();
	std::array<unsigned char, 32> _key{};

	void _crypt(span<std::uint8_t*> buffer, span<std::uint8_t*> data, bool encrypt)
	{
		EVP_CIPHER_CTX_reset(_context);

		if (!EVP_CipherInit_ex(_context, _cipher, nullptr, _key.data(), data.begin(), encrypt)) {
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
