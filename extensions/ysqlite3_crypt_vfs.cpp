#include <openssl/evp.h>
#include <openssl/rand.h>
#include <ysqlite3/vfs/layer/layer.hpp>
#include <ysqlite3/vfs/layer/layered_vfs.hpp>
#include <ysqlite3/vfs/sqlite3_vfs_wrapper.hpp>

// clang-format
#include <ysqlite3/sqlite3ext.h>
SQLITE_EXTENSION_INIT1

using namespace ysqlite3;

class encryptor : public vfs::layer::layer
{
public:
	encryptor()
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

static std::shared_ptr<vfs::layer::layered_vfs<vfs::sqlite3_vfs_wrapper<>, vfs::layer::layered_file>>
    crypt_vfs;

#ifdef _WIN32
__declspec(dllexport)
#endif
    extern "C" int sqlite3_ysqlitecryptvfs_init(sqlite3* db, char** error_message,
                                                const sqlite3_api_routines* api) noexcept
{
	int rc = SQLITE_OK;
	SQLITE_EXTENSION_INIT2(api);

	try {
		crypt_vfs =
		    std::make_shared<decltype(crypt_vfs)::element_type>(vfs::find_vfs(nullptr), "ysqlite3-crypt-vfs");
		crypt_vfs->add_layer<encryptor>();
		vfs::register_vfs(crypt_vfs, true);
	} catch (...) {
		return SQLITE_ERROR;
	}

	return rc == SQLITE_OK ? SQLITE_OK_LOAD_PERMANENTLY : rc;
}
