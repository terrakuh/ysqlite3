#ifndef YSQLITE_VFS_FILE_HPP_
#define YSQLITE_VFS_FILE_HPP_

#include "../exception/sqlite3_exception.hpp"
#include "../sqlite3.h"

#include <gsl/gsl>

namespace ysqlite3 {
namespace vfs {

class file
{
public:
	enum class file_format
	{
		main_db        = SQLITE_OPEN_MAIN_DB,
		main_journal   = SQLITE_OPEN_MAIN_JOURNAL,
		temp_db        = SQLITE_OPEN_TEMP_DB,
		temp_journal   = SQLITE_OPEN_TEMP_JOURNAL,
		transient_db   = SQLITE_OPEN_TRANSIENT_DB,
		subjournal     = SQLITE_OPEN_SUBJOURNAL,
		master_journal = SQLITE_OPEN_MASTER_JOURNAL,
		wal            = SQLITE_OPEN_WAL
	};

	enum class sync_flag
	{
		normal    = SQLITE_SYNC_NORMAL,
		full      = SQLITE_SYNC_FULL,
		data_only = SQLITE_SYNC_DATAONLY
	};

	enum class lock_flag
	{
		none      = SQLITE_LOCK_NONE,
		shared    = SQLITE_LOCK_SHARED,
		reserved  = SQLITE_LOCK_RESERVED,
		pending   = SQLITE_LOCK_PENDING,
		exclusive = SQLITE_LOCK_EXCLUSIVE,
	};

	enum file_cntl
	{
		file_cntl_lock_state            = SQLITE_FCNTL_LOCKSTATE,
		file_cntl_get_lock_proxy_file   = SQLITE_FCNTL_GET_LOCKPROXYFILE,
		file_cntl_set_lock_proxy_file   = SQLITE_FCNTL_SET_LOCKPROXYFILE,
		file_cntl_last_errno            = SQLITE_FCNTL_LAST_ERRNO,
		file_cntl_size_hint             = SQLITE_FCNTL_SIZE_HINT,
		file_cntl_chunk_size            = SQLITE_FCNTL_CHUNK_SIZE,
		file_cntl_file_pointer          = SQLITE_FCNTL_FILE_POINTER,
		file_cntl_sync_omitted          = SQLITE_FCNTL_SYNC_OMITTED,
		file_cntl_win32_av_retry        = SQLITE_FCNTL_WIN32_AV_RETRY,
		file_cntl_persist_wal           = SQLITE_FCNTL_PERSIST_WAL,
		file_cntl_overwrite             = SQLITE_FCNTL_OVERWRITE,
		file_cntl_vfsname               = SQLITE_FCNTL_VFSNAME,
		file_cntl_powersafe_overwrite   = SQLITE_FCNTL_POWERSAFE_OVERWRITE,
		file_cntl_pragma                = SQLITE_FCNTL_PRAGMA,
		file_cntl_busy_handler          = SQLITE_FCNTL_BUSYHANDLER,
		file_cntl_temp_filename         = SQLITE_FCNTL_TEMPFILENAME,
		file_cntl_mmap_size             = SQLITE_FCNTL_MMAP_SIZE,
		file_cntl_trace                 = SQLITE_FCNTL_TRACE,
		file_cntl_has_moved             = SQLITE_FCNTL_HAS_MOVED,
		file_cntl_sync                  = SQLITE_FCNTL_SYNC,
		file_cntl_commit_phase_two      = SQLITE_FCNTL_COMMIT_PHASETWO,
		file_cntl_win32_set_handle      = SQLITE_FCNTL_WIN32_SET_HANDLE,
		file_cntl_wal_block             = SQLITE_FCNTL_WAL_BLOCK,
		file_cntl_zipvfs                = SQLITE_FCNTL_ZIPVFS,
		file_cntl_rbu                   = SQLITE_FCNTL_RBU,
		file_cntl_vfs_pointer           = SQLITE_FCNTL_VFS_POINTER,
		file_cntl_journal_pointer       = SQLITE_FCNTL_JOURNAL_POINTER,
		file_cntl_win32_get_handle      = SQLITE_FCNTL_WIN32_GET_HANDLE,
		file_cntl_pdf                   = SQLITE_FCNTL_PDB,
		file_cntl_begin_atomic_write    = SQLITE_FCNTL_BEGIN_ATOMIC_WRITE,
		file_cntl_commit_atomic_write   = SQLITE_FCNTL_COMMIT_ATOMIC_WRITE,
		file_cntl_rollback_atomic_write = SQLITE_FCNTL_ROLLBACK_ATOMIC_WRITE,
		file_cntl_lock_timeout          = SQLITE_FCNTL_LOCK_TIMEOUT,
		file_cntl_data_version          = SQLITE_FCNTL_DATA_VERSION,
		file_cntl_size_limit            = SQLITE_FCNTL_SIZE_LIMIT
	};

	/**
	 Constructor.
	*/
	file() noexcept : _methods{}
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
	}
	virtual ~file() = default;
	sqlite3_io_methods* methods() noexcept
	{
		return &_methods;
	}
	virtual void close()                                                        = 0;
	virtual void read(gsl::span<gsl::byte> buffer, sqlite3_int64 offset)        = 0;
	virtual void write(gsl::span<const gsl::byte> buffer, sqlite3_int64 offset) = 0;
	virtual void truncate(sqlite3_int64 size)                                   = 0;
	virtual void sync(sync_flag flag)                                           = 0;
	virtual sqlite3_int64 file_size() const                                     = 0;
	virtual void lock(lock_flag flag)                                           = 0;
	virtual void unlock(lock_flag flag)                                         = 0;
	virtual bool has_reserved_lock() const                                      = 0;
	virtual void file_control(file_cntl operation, void* arg)                   = 0;
	/**
	 Returns the sector size for this file. The default implementation returns 4096.

	 @returns the sector size
	*/
	virtual int sector_size() const noexcept
	{
		return 4096;
	}
	/**
	 Returns SQLite specific device characteristics.

	 @returns device characteristics
	*/
	virtual int device_characteristics() const noexcept = 0;

private:
	sqlite3_io_methods _methods;

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
		return _wrap([=] { _self(file)->write(gsl::make_span(static_cast<const gsl::byte*>(buffer), size), offset); });
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
};

} // namespace vfs
} // namespace ysqlite3

#endif // !YSQLITE_VFS_FILE_HPP_
