#include <array>
#include <cstring>
#include <iostream>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <typeinfo>
#include <ysqlite3/database.hpp>
#include <ysqlite3/vfs/page_transforming_file.hpp>
#include <ysqlite3/vfs/sqlite3_vfs_wrapper.hpp>
#include <ysqlite3/vfs/vfs.hpp>

using namespace ysqlite3;

class crypt_file : public vfs::page_transforming_file<ysqlite3::vfs::sqlite3_file_wrapper>
{
public:
	using page_transformer::page_transformer;
	~crypt_file()
	{
		EVP_CIPHER_CTX_free(_context);
	}
	void file_control(vfs::file_cntl operation, void* arg) override
	{
		if (operation == vfs::file_cntl::pragma) {
			const auto& name = static_cast<char**>(arg)[1];
			const auto value = static_cast<char**>(arg)[2];

			if (!std::strcmp(name, "plain_key")) {
				if (!PKCS5_PBKDF2_HMAC(value, -1, nullptr, 0, 200000, EVP_sha512(),
				                       static_cast<int>(_key.size()), _key.data())) {
					throw std::system_error{ sqlite3_errc::generic, "failed to generate new key" };
				}
				return;
			}
		}

		page_transformer::file_control(operation, arg);
	}
	constexpr static std::uint8_t reserve_size() noexcept
	{
		return 28;
	}

protected:
	void encode_page(span<std::uint8_t*> page) override
	{
		auto data = page.subspan(page.size() - reserve_size());
		page      = page.subspan(0, page.size() - reserve_size());
		std::memset(data.begin(), 0, data.size());

		// _assert_params();
		// const auto iv_length = EVP_CIPHER_iv_length(_cipher);
		// RAND_bytes(data.begin(), iv_length);
		// _crypt(page, data, true);
		// EVP_CIPHER_CTX_ctrl(_context, EVP_CTRL_GCM_GET_TAG, 16, data.begin() + iv_length);
	}
	void decode_page(span<std::uint8_t*> page) override
	{
		auto data = page.subspan(page.size() - reserve_size());
		std::memset(data.begin(), 0, data.size());
		// _assert_params();
		// _crypt(page.subspan(0, page.size() - data_size), page.subspan(page.size() - data_size), false);
	}
	bool check_reserve_size(std::uint8_t size) const noexcept override
	{
		return size == reserve_size();
	}

private:
	EVP_CIPHER_CTX* _context  = EVP_CIPHER_CTX_new();
	const EVP_CIPHER* _cipher = EVP_aes_256_gcm();
	std::array<unsigned char, 32> _key{};

	void _assert_params()
	{
		// if (!_key) {
		// 	throw std::system_error{ sqlite3_errc::generic, "no key provided" };
		// }
	}
	void _crypt(span<std::uint8_t*> buffer, span<std::uint8_t*> data, bool encrypt)
	{
		EVP_CIPHER_CTX_reset(_context);

		if (!EVP_CipherInit_ex(_context, _cipher, nullptr, _key.data(), data.begin(), encrypt)) {
			throw;
		}

		int len = 0;
		if (!EVP_CipherUpdate(_context, buffer.begin(), &len, buffer.begin(),
		                      static_cast<int>(buffer.size())) ||
		    len != static_cast<int>(buffer.size())) {
			throw;
		}

		// set tag
		if (!encrypt) {
			EVP_CIPHER_CTX_ctrl(_context, EVP_CTRL_GCM_SET_TAG, 16, data.begin() + 12);
		}

		if (!EVP_CipherFinal_ex(_context, buffer.begin(), &len) || len) {
			throw;
		}
	}
	void _generate_iv(span<std::uint8_t*> iv) const noexcept
	{
		RAND_bytes(reinterpret_cast<unsigned char*>(iv.begin()), static_cast<int>(iv.size() - 4));
		*reinterpret_cast<std::uint32_t*>(iv.end() - 4) += 1;
#if YSQLITE3_BIG_ENDIAN
		std::reverse(iv.end() - 4, iv.end());
#endif
	}
};

int main(int args, char** argv)
try {
	vfs::register_vfs(
	    std::make_shared<vfs::sqlite3_vfs_wrapper<crypt_file>>(vfs::find_vfs(nullptr), "aes-gcm"), true);

	database db;
	std::remove("test.db");
	std::remove("test.db-journal");
	std::remove("test.db-wal");
	db.open("test.db", open_flag_readwrite | open_flag_create, "aes-gcm");
	db.set_reserved_size(crypt_file::reserve_size());
	// db.set_journal_mode(journal_mode::memory);
	db.execute(R"(

CREATE TABLE IF NOT EXISTS tast(noim text);
INSERT INTO tast(noim) VALUES('heyho');

)");
	db.execute("BEGIN TRANSACTION");
	for (auto i = 0; i < 300; ++i) {
		db.execute("INSERT INTO tast(noim) VALUES('heyho');");
	}
	db.execute("COMMIT");

	auto stmt = db.prepare_statement("SELECT * FROM tast");
	results r;

	while ((r = stmt.step())) {
		for (const auto& column : stmt.columns()) {
			const auto text = r.text(column.c_str());
			std::cout << column << ": " << (!text ? "<null>" : text) << '\n';
		}
	}
} catch (const std::system_error& e) {
	std::cerr << "exception caught (ec=" << e.code() << "):\n\t" << e.what() << '\n';
	return -1;
}
