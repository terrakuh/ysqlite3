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
    extern "C" int sqlite3_ysqlitecryptvfs_init(sqlite3* db, char** error_message,
                                                const sqlite3_api_routines* api) noexcept
{
	int rc = SQLITE_OK;
	SQLITE_EXTENSION_INIT2(api);

	try {
		vfs::register_vfs(
		    std::make_shared<vfs::sqlite3_vfs_wrapper<vfs::crypt_file<vfs::sqlite3_file_wrapper>>>(
		        vfs::find_vfs(nullptr), "ysqlite3-crypt-vfs"),
		    false);
		int n = vfs::crypt_file_reserve_size();
		rc    = sqlite3_file_control(db, nullptr, SQLITE_FCNTL_RESERVE_BYTES, &n);
	} catch (...) {
		return SQLITE_ERROR;
	}

	return rc == SQLITE_OK ? SQLITE_OK_LOAD_PERMANENTLY : rc;
}
