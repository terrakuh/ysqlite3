#include <ysqlite3/vfs/crypt_file.hpp>
#include <ysqlite3/vfs/sqlite3_file_wrapper.hpp>
#include <ysqlite3/vfs/sqlite3_vfs_wrapper.hpp>

using namespace ysqlite3;

extern "C" int ysqlite3_init() noexcept
{
	try {
		vfs::register_vfs(
		    std::make_shared<vfs::sqlite3_vfs_wrapper<vfs::crypt_file<vfs::sqlite3_file_wrapper>>>(
		        vfs::find_vfs(nullptr), "ysqlite3-crypt-vfs"),
		    false);
	} catch (...) {
		return SQLITE_ERROR;
	}
	return SQLITE_OK;
}
