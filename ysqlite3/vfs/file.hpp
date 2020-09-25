#ifndef YSQLITE_VFS_FILE_HPP_
#define YSQLITE_VFS_FILE_HPP_

#include "../span.hpp"
#include "../sqlite3.h"

#include <cstdint>

namespace ysqlite3 {
namespace vfs {

enum class file_format
{
	main_db        = SQLITE_OPEN_MAIN_DB,
	main_journal   = SQLITE_OPEN_MAIN_JOURNAL,
	temp_db        = SQLITE_OPEN_TEMP_DB,
	temp_journal   = SQLITE_OPEN_TEMP_JOURNAL,
	transient_db   = SQLITE_OPEN_TRANSIENT_DB,
	subjournal     = SQLITE_OPEN_SUBJOURNAL,
	super_journal  = SQLITE_OPEN_SUPER_JOURNAL,
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

class file
{
public:
	/**
	 Constructor.
	*/
	file(file_format format) noexcept;
	virtual ~file() = default;
	sqlite3_io_methods* methods() noexcept
	{
		return &_methods;
	}
	virtual void close()                                                       = 0;
	virtual void read(span<std::uint8_t*> buffer, sqlite3_int64 offset)        = 0;
	virtual void write(span<const std::uint8_t*> buffer, sqlite3_int64 offset) = 0;
	virtual void truncate(sqlite3_int64 size)                                  = 0;
	virtual void sync(sync_flag flag)                                          = 0;
	virtual sqlite3_int64 file_size() const                                    = 0;
	virtual void lock(lock_flag flag)                                          = 0;
	virtual void unlock(lock_flag flag)                                        = 0;
	virtual bool has_reserved_lock() const                                     = 0;
	virtual void file_control(file_cntl operation, void* arg)                  = 0;
	/**
	 * Returns the sector size for this file. The default implementation returns 4096.
	 *
	 * @return the sector size
	 */
	virtual int sector_size() const noexcept
	{
		return 4096;
	}
	/**
	 * Returns SQLite specific device characteristics.
	 *
	 * @return device characteristics
	 */
	virtual int device_characteristics() const noexcept = 0;

protected:
	const file_format format;

private:
	sqlite3_io_methods _methods;
};

} // namespace vfs
} // namespace ysqlite3

#endif
