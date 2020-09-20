#include "vfs.hpp"

using namespace ysqlite3::vfs;

inline vfs* self(sqlite3_vfs* vfs) noexcept
{
	return static_cast<class vfs*>(vfs->pAppData);
}

template<typename Action>
inline int wrap(Action&& action) noexcept
{
	try {
		action();
		return SQLITE_OK;
	} catch (const exception::sqlite3_exception& e) {
		return e.error_code();
	} catch (...) {
		return SQLITE_ERROR;
	}
}

inline int open(sqlite3_vfs* vfs, const char* name, sqlite3_file* file, int flags, int* out_flags) noexcept
{
	// clear in case of an error
	std::memset(file, 0, vfs->szOsFile);
	return wrap([=] {
		int tmp = 0;
		file::format format;
		constexpr auto mask = SQLITE_OPEN_MAIN_DB | SQLITE_OPEN_TEMP_DB | SQLITE_OPEN_TRANSIENT_DB |
		                      SQLITE_OPEN_MAIN_JOURNAL | SQLITE_OPEN_TEMP_JOURNAL | SQLITE_OPEN_SUBJOURNAL |
		                      SQLITE_OPEN_MASTER_JOURNAL | SQLITE_OPEN_WAL;

		switch (flags & mask) {
		case SQLITE_OPEN_MAIN_DB:
		case SQLITE_OPEN_TEMP_DB:
		case SQLITE_OPEN_TRANSIENT_DB:
		case SQLITE_OPEN_MAIN_JOURNAL:
		case SQLITE_OPEN_TEMP_JOURNAL:
		case SQLITE_OPEN_SUBJOURNAL:
		case SQLITE_OPEN_MASTER_JOURNAL:
		case SQLITE_OPEN_WAL: format = static_cast<file::format>(flags & mask); break;
		default: throw std::system_error{ ysqlite3::sqlite3::errc::misuse, "unknown file format" };
		}

		// open
		const auto f   = self(vfs)->open(name, format, flags, out_flags ? *out_flags : tmp);
		file->pMethods = f->methods();
		*reinterpret_cast<void**>(file + 1) = f;
	});
}
static int _delete(sqlite3_vfs* vfs, const char* name, int sync_directory) noexcept
{
	return _wrap([=] { _self(vfs)->delete_file(name, static_cast<bool>(sync_directory)); });
}
static int _access(sqlite3_vfs* vfs, const char* name, int flags, int* result) noexcept
{
	try {
		access_flag flag;

		switch (flags) {
		case SQLITE_ACCESS_EXISTS:
		case SQLITE_ACCESS_READWRITE:
		case SQLITE_ACCESS_READ: flag = static_cast<access_flag>(flags); break;
		default: return SQLITE_IOERR_ACCESS;
		}

		*result = _self(vfs)->access(name, flag);

		return SQLITE_OK;
	} catch (...) {
		return SQLITE_IOERR_ACCESS;
	}
}
static int _full_pathname(sqlite3_vfs* vfs, const char* name, int size, char* buffer) noexcept
{
	try {
		_self(vfs)->full_pathname(name, { buffer, size });

		return SQLITE_OK;
	} catch (...) {
		return SQLITE_ERROR;
	}
}
static void* _dlopen(sqlite3_vfs* vfs, const char* filename) noexcept
{
	return _self(vfs)->dlopen(filename);
}
static void _dlerror(sqlite3_vfs* vfs, int size, char* buffer) noexcept
{
	_self(vfs)->dlerror({ buffer, size });
}
static dlsym_type _dlsym(sqlite3_vfs* vfs, void* handle, const char* symbol) noexcept
{
	if (handle) {
		return _self(vfs)->dlsym(handle, symbol);
	}

	return nullptr;
}
static void _dlclose(sqlite3_vfs* vfs, void* handle) noexcept
{
	if (handle) {
		return _self(vfs)->dlclose(handle);
	}
}
static int _random(sqlite3_vfs* vfs, int size, char* buffer) noexcept
{
	return _self(vfs)->random(gsl::as_writeable_bytes(gsl::make_span(buffer, size)));
}
static int _sleep(sqlite3_vfs* vfs, int microseconds) noexcept
{
	return _self(vfs)->sleep(sleep_duration_type(microseconds)).count();
}
static int _current_time(sqlite3_vfs* vfs, double* time) noexcept
{
	sqlite3_int64 t = 0;
	auto rc         = _current_time64(vfs, &t);

	*time = t / 86400000.0;

	return rc;
}
static int _last_error(sqlite3_vfs* vfs, int size, char* buffer) noexcept
{
	return _self(vfs)->last_error({ buffer, size });
}
static int _current_time64(sqlite3_vfs* vfs, sqlite3_int64* time) noexcept
{
	try {
		*time = _self(vfs)->current_time().count();

		return SQLITE_OK;
	} catch (...) {
		return SQLITE_ERROR;
	}
}
static int _set_system_call(sqlite3_vfs* vfs, const char* name, sqlite3_syscall_ptr system_call) noexcept
{
	return _self(vfs)->set_system_call(name, system_call);
}
static sqlite3_syscall_ptr _get_system_call(sqlite3_vfs* vfs, const char* name) noexcept
{
	return _self(vfs)->get_system_call(name);
}
static const char* _next_system_call(sqlite3_vfs* vfs, const char* name) noexcept
{
	return _self(vfs)->next_system_call(name);
}
