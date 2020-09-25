#include <iostream>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <typeinfo>
#include <ysqlite3/database.hpp>
#include <ysqlite3/vfs/layer/layered_vfs.hpp>
#include <ysqlite3/vfs/sqlite3_vfs_wrapper.hpp>
#include <ysqlite3/vfs/vfs.hpp>

using namespace ysqlite3;

class aes_gcm_layer : public vfs::layer::layer
{
public:
	aes_gcm_layer()
	{
		_context = EVP_CIPHER_CTX_new();
		_cipher  = EVP_aes_128_gcm();
		_key     = (const unsigned char*) "1234567890123456";
	}
	void encode(span<std::uint8_t*> page, span<std::uint8_t*> data) override
	{
		RAND_bytes(data.begin(), 12);
		_crypt(page, data.cast<const std::uint8_t*>(), true);
		EVP_CIPHER_CTX_ctrl(_context, EVP_CTRL_GCM_GET_TAG, 16, data.begin() + 12);
	}
	void decode(span<std::uint8_t*> page, span<const std::uint8_t*> data) override
	{
		_crypt(page, data, false);
	}
	std::uint8_t data_size() const noexcept override
	{
		return 28;
	}
	bool pragma(const char* name, const char* value) override
	{
		printf("doing: %s=%s\n", name, value);
		return true;
	}

private:
	EVP_CIPHER_CTX* _context;
	const EVP_CIPHER* _cipher;
	const unsigned char* _key;

	void _crypt(span<std::uint8_t*> buffer, span<const std::uint8_t*> data, bool encrypt)
	{
		EVP_CIPHER_CTX_reset(_context);

		if (!EVP_CipherInit_ex(_context, _cipher, nullptr, _key, data.begin(), encrypt)) {
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
			EVP_CIPHER_CTX_ctrl(_context, EVP_CTRL_GCM_SET_TAG, 16,
			                    const_cast<std::uint8_t*>(data.begin()) + 12);
		}

		if (!EVP_CipherFinal_ex(_context, buffer.begin(), &len) || len) {
			throw;
		}
	}
};

int main(int args, char** argv)
try {
	const auto v =
	    std::make_shared<vfs::layer::layered_vfs<vfs::sqlite3_vfs_wrapper<>, vfs::layer::layered_file>>(
	        vfs::find_vfs(nullptr), "aes-gcm");
	v->add_layer<aes_gcm_layer>();
	vfs::register_vfs(v, true);
auto p = new database;
	database& db=*p;
	std::remove("test.db");
	std::remove("test.db-journal");
	std::remove("test.db-wal");
	db.open("test.db", open_flag_readwrite | open_flag_create, "aes-gcm");
	// db.set_journal_mode(journal_mode::delete_);
	db.execute(R"(
PRAGMA key="hi";
BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS tast(noim text);
INSERT INTO tast(noim) VALUES('heyho'), ('Musik'), (NULL);
COMMIT;
BEGIN TRANSACTION;
INSERT INTO tast(noim) VALUES('heyho'), ('Musik'), (NULL);

)");

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
