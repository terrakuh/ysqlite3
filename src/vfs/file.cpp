#include "file.hpp"

using namespace ysqlite3::vfs;

static file* _self(sqlite3_file* file) noexcept
{
	return *reinterpret_cast<class file**>(file + 1);
}
template<typename T>
static int _wrap(T&& action, int default_error = SQLITE_ERROR) noexcept
{
	try {
		action();

		return SQLITE_OK;
	} catch (const exception::sqlite3_exception& e) {
		return e.error_code();
	} catch (...) {
		return default_error;
	}
}
static lock_flag _lock_flag(int flag)
{
	switch (flag) {
	case SQLITE_LOCK_NONE:
	case SQLITE_LOCK_SHARED:
	case SQLITE_LOCK_RESERVED:
	case SQLITE_LOCK_PENDING:
	case SQLITE_LOCK_EXCLUSIVE: return static_cast<lock_flag>(flag);
	default: YSQLITE_THROW(exception::sqlite3_exception, SQLITE_MISUSE, "unknown lock flag");
	}
}
static int _close(sqlite3_file* file) noexcept
{
	return _wrap([file] {
		auto self = _self(file);
		auto _    = gsl::finally([self] { delete self; });

		self->close();
	});
}
static int _read(sqlite3_file* file, void* buffer, int size, sqlite3_int64 offset) noexcept
{
	return _wrap([=] { _self(file)->read(gsl::make_span(static_cast<gsl::byte*>(buffer), size), offset); });
}
static int _write(sqlite3_file* file, const void* buffer, int size, sqlite3_int64 offset) noexcept
{
	return _wrap(
	    [=] { _self(file)->write(gsl::make_span(static_cast<const gsl::byte*>(buffer), size), offset); });
}
static int _truncate(sqlite3_file* file, sqlite3_int64 size) noexcept
{
	return _wrap([=] { _self(file)->truncate(size); });
}
static int _sync(sqlite3_file* file, int flag) noexcept
{
	return _wrap([=] {
		sync_flag sf;

		switch (flag) {
		case SQLITE_SYNC_NORMAL:
		case SQLITE_SYNC_FULL:
		case SQLITE_SYNC_DATAONLY: sf = static_cast<sync_flag>(flag); break;
		default: YSQLITE_THROW(exception::sqlite3_exception, SQLITE_MISUSE, "unknown sync flag");
		}

		_self(file)->sync(sf);
	});
}
static int _file_size(sqlite3_file* file, sqlite3_int64* size) noexcept
{
	return _wrap([=] { *size = _self(file)->file_size(); });
}
static int _lock(sqlite3_file* file, int flag) noexcept
{
	return _wrap([=] { _self(file)->lock(_lock_flag(flag)); }, SQLITE_IOERR_LOCK);
}
static int _unlock(sqlite3_file* file, int flag) noexcept
{
	return _wrap([=] { _self(file)->lock(_lock_flag(flag)); }, SQLITE_IOERR_UNLOCK);
}
static int _check_reserved_lock(sqlite3_file* file, int* out) noexcept
{
	return _wrap(
	    [=] {
		    *out = 0;
		    *out = _self(file)->has_reserved_lock();
	    },
	    SQLITE_IOERR_CHECKRESERVEDLOCK);
}
static int _file_control(sqlite3_file* file, int operation, void* arg) noexcept
{
	return _wrap([=] { _self(file)->file_control(static_cast<file_cntl>(operation), arg); });
}
static int _sector_size(sqlite3_file* file) noexcept
{
	return _self(file)->sector_size();
}
static int _device_characteristics(sqlite3_file* file) noexcept
{
	return _self(file)->device_characteristics();
}

/*static int _shm_map(sqlite3_file*, int iPg, int pgsz, int, void volatile**);
static int _shm_lock(sqlite3_file*, int offset, int n, int flags);
static void _shm_barrier(sqlite3_file*);
static int _shm_unmap(sqlite3_file*, int deleteFlag);

static int _fetch(sqlite3_file*, sqlite3_int64 iOfst, int iAmt, void** pp);
static int _unfetch(sqlite3_file*, sqlite3_int64 iOfst, void* p);
/* Methods above are valid for version 3 */
/* Additional methods may be added in future releases */

file::file(file_format format) noexcept : _methods{}
{
	_methods.iVersion               = 1;
	_methods.xClose                 = _close;
	_methods.xRead                  = _read;
	_methods.xWrite                 = _write;
	_methods.xTruncate              = _truncate;
	_methods.xSync                  = _sync;
	_methods.xFileSize              = _file_size;
	_methods.xLock                  = _lock;
	_methods.xUnlock                = _unlock;
	_methods.xCheckReservedLock     = _check_reserved_lock;
	_methods.xFileControl           = _file_control;
	_methods.xSectorSize            = _sector_size; // opt
	_methods.xDeviceCharacteristics = _device_characteristics;

	/*_methods.xShmLock    = _shm_lock; // for wal
	_methods.xShmMap     = _shm_map;
	_methods.xShmBarrier = _shm_barrier;
	_methods.xShmUnmap   = _shm_unmap;

	_methods.xFetch   = _fetch; // if SQLITE_MAX_MMAP_SIZE > 0
	_methods.xUnfetch = _unfetch;*/
	_format = format;
}
