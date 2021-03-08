#include <ysqlite3/vfs/crypt_file.hpp>
#include <ysqlite3/vfs/sqlite3_file_wrapper.hpp>
#include <ysqlite3/vfs/sqlite3_vfs_wrapper.hpp>

// clang-format
#include <ysqlite3/sqlite3ext.h>
SQLITE_EXTENSION_INIT1

using namespace ysqlite3;

#ifdef _WIN32
__declspec(dllexport)
#endif
    extern "C" int ysqlite3_init_crypt_db(sqlite3* db, char** error_message,
                                          const sqlite3_api_routines* api) noexcept
{
	SQLITE_EXTENSION_INIT2(api);
	int n   = vfs::crypt_file_reserve_size();
	auto ec = sqlite3_file_control(db, nullptr, SQLITE_FCNTL_RESERVE_BYTES, &n);
	if (!ec && n != vfs::crypt_file_reserve_size()) {
		ec = sqlite3_exec(db, "VACUUM", nullptr, nullptr, nullptr);
		if (ec) {
			sqlite3_file_control(db, nullptr, SQLITE_FCNTL_RESERVE_BYTES, &n);
		}
	}
	return ec;
}

#ifdef _WIN32
__declspec(dllexport)
#endif
    extern "C" int ysqlite3_register_crypt_vfs(sqlite3* db, char** error_message,
                                               const sqlite3_api_routines* api) noexcept
{
	SQLITE_EXTENSION_INIT2(api);

	// check version
	if (sqlite3_libversion_number() < 3032000) {
		*error_message = sqlite3_mprintf("incompatible SQLite3 version; min 3.32.0");
		return SQLITE_ERROR;
	}

	try {
		vfs::register_vfs(
		    std::make_shared<vfs::SQLite3_vfs_wrapper<vfs::Crypt_file<vfs::SQLite3_file_wrapper>>>(
		        vfs::find_vfs(nullptr), YSQLITE3_CRYPT_VFS_NAME),
		    false);
	} catch (...) {
		return SQLITE_ERROR;
	}

	return SQLITE_OK_LOAD_PERMANENTLY;
}
