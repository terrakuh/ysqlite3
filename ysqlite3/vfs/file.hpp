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

enum class file_cntl
{
	lock_state            = SQLITE_FCNTL_LOCKSTATE,
	get_lock_proxy_file   = SQLITE_FCNTL_GET_LOCKPROXYFILE,
	set_lock_proxy_file   = SQLITE_FCNTL_SET_LOCKPROXYFILE,
	last_errno            = SQLITE_FCNTL_LAST_ERRNO,
	size_hint             = SQLITE_FCNTL_SIZE_HINT,
	chunk_size            = SQLITE_FCNTL_CHUNK_SIZE,
	file_pointer          = SQLITE_FCNTL_FILE_POINTER,
	sync_omitted          = SQLITE_FCNTL_SYNC_OMITTED,
	win32_av_retry        = SQLITE_FCNTL_WIN32_AV_RETRY,
	persist_wal           = SQLITE_FCNTL_PERSIST_WAL,
	overwrite             = SQLITE_FCNTL_OVERWRITE,
	vfsname               = SQLITE_FCNTL_VFSNAME,
	powersafe_overwrite   = SQLITE_FCNTL_POWERSAFE_OVERWRITE,
	pragma                = SQLITE_FCNTL_PRAGMA,
	busy_handler          = SQLITE_FCNTL_BUSYHANDLER,
	temp_filename         = SQLITE_FCNTL_TEMPFILENAME,
	mmap_size             = SQLITE_FCNTL_MMAP_SIZE,
	trace                 = SQLITE_FCNTL_TRACE,
	has_moved             = SQLITE_FCNTL_HAS_MOVED,
	sync                  = SQLITE_FCNTL_SYNC,
	commit_phase_two      = SQLITE_FCNTL_COMMIT_PHASETWO,
	win32_set_handle      = SQLITE_FCNTL_WIN32_SET_HANDLE,
	wal_block             = SQLITE_FCNTL_WAL_BLOCK,
	zipvfs                = SQLITE_FCNTL_ZIPVFS,
	rbu                   = SQLITE_FCNTL_RBU,
	vfs_pointer           = SQLITE_FCNTL_VFS_POINTER,
	journal_pointer       = SQLITE_FCNTL_JOURNAL_POINTER,
	win32_get_handle      = SQLITE_FCNTL_WIN32_GET_HANDLE,
	pdf                   = SQLITE_FCNTL_PDB,
	begin_atomic_write    = SQLITE_FCNTL_BEGIN_ATOMIC_WRITE,
	commit_atomic_write   = SQLITE_FCNTL_COMMIT_ATOMIC_WRITE,
	rollback_atomic_write = SQLITE_FCNTL_ROLLBACK_ATOMIC_WRITE,
	lock_timeout          = SQLITE_FCNTL_LOCK_TIMEOUT,
	data_version          = SQLITE_FCNTL_DATA_VERSION,
	size_limit            = SQLITE_FCNTL_SIZE_LIMIT,
	checkpoint_done       = SQLITE_FCNTL_CKPT_DONE,
	reserve_bytes         = SQLITE_FCNTL_RESERVE_BYTES,
	checkpoint_start      = SQLITE_FCNTL_CKPT_START,
};

inline const char* name_of(file_format format) noexcept
{
	switch (format) {
	case file_format::main_db: return "main db";
	case file_format::main_journal: return "main journal";
	default: return "unkown";
	}
}

class file
{
public:
	/**
	 Constructor.
	*/
	file(const char* name, file_format format) noexcept;
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
	virtual int device_characteristics() const noexcept                                         = 0;
	virtual void shm_map(int page, int page_size, bool is_write, void volatile** mapped_memory) = 0;
	virtual void shm_lock(int offset, int n, int flags)                                         = 0;
	virtual void shm_barrier() noexcept                                                         = 0;
	virtual void shm_unmap(int delete_flag)                                                     = 0;
	virtual void fetch(sqlite3_int64 offset, int amount, void** buffer)                         = 0;
	virtual void unfetch(sqlite3_int64 offset, void* buffer)                                    = 0;

protected:
	const char* const name;
	const file_format format;

private:
	sqlite3_io_methods _methods;
};

} // namespace vfs
} // namespace ysqlite3

#endif
