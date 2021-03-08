#include "file.hpp"

#include "../error.hpp"
#include "../finally.hpp"

using namespace ysqlite3::vfs;

namespace {

File* self(sqlite3_file* file) noexcept
{
	return *reinterpret_cast<class File**>(file + 1);
}

template<typename Action>
int wrap(Action&& action, int default_error = SQLITE_ERROR) noexcept
{
	try {
		action();
		return SQLITE_OK;
	} catch (const std::system_error& e) {
		return e.code().category() == ysqlite3::sqlite3_category() ? e.code().value() : default_error;
	} catch (...) {
		return default_error;
	}
}

Lock_flag lock_flag(int flag)
{
	switch (flag) {
	case SQLITE_LOCK_NONE:
	case SQLITE_LOCK_SHARED:
	case SQLITE_LOCK_RESERVED:
	case SQLITE_LOCK_PENDING:
	case SQLITE_LOCK_EXCLUSIVE: return static_cast<Lock_flag>(flag);
	default: throw std::system_error{ ysqlite3::SQLite3_code::library_misuse };
	}
}

int close(sqlite3_file* file) noexcept
{
	return wrap([file] {
		const auto self = ::self(file);
		const auto _    = ysqlite3::finally([self] { delete self; });
		self->close();
	});
}

int read(sqlite3_file* file, void* buffer, int size, sqlite3_int64 offset) noexcept
{
	return wrap([&] {
		self(file)->read({ static_cast<std::uint8_t*>(buffer), static_cast<std::size_t>(size) }, offset);
	});
}

int write(sqlite3_file* file, const void* buffer, int size, sqlite3_int64 offset) noexcept
{
	return wrap([&] {
		self(file)->write({ static_cast<const std::uint8_t*>(buffer), static_cast<std::size_t>(size) },
		                  offset);
	});
}

int truncate(sqlite3_file* file, sqlite3_int64 size) noexcept
{
	return wrap([&] { self(file)->truncate(size); });
}

int sync(sqlite3_file* file, int flag) noexcept
{
	return wrap([&] {
		Sync_flag sf;
		switch (flag) {
		case SQLITE_SYNC_NORMAL:
		case SQLITE_SYNC_FULL:
		case SQLITE_SYNC_DATAONLY: sf = static_cast<Sync_flag>(flag); break;
		default: throw std::system_error{ ysqlite3::SQLite3_code::library_misuse };
		}
		self(file)->sync(sf);
	});
}

int file_size(sqlite3_file* file, sqlite3_int64* size) noexcept
{
	return wrap([&] { *size = self(file)->file_size(); });
}

int lock(sqlite3_file* file, int flag) noexcept
{
	return wrap([&] { self(file)->lock(lock_flag(flag)); }, SQLITE_IOERR_LOCK);
}

int unlock(sqlite3_file* file, int flag) noexcept
{
	return wrap([&] { self(file)->unlock(lock_flag(flag)); }, SQLITE_IOERR_UNLOCK);
}

int check_reserved_lock(sqlite3_file* file, int* out) noexcept
{
	return wrap(
	    [&] {
		    *out = 0;
		    *out = self(file)->has_reserved_lock();
	    },
	    SQLITE_IOERR_CHECKRESERVEDLOCK);
}

int file_control(sqlite3_file* file, int operation, void* arg) noexcept
{
	return wrap([&] { self(file)->file_control(static_cast<File_control>(operation), arg); });
}

int sector_size(sqlite3_file* file) noexcept
{
	return self(file)->sector_size();
}

int device_characteristics(sqlite3_file* file) noexcept
{
	return self(file)->device_characteristics();
}

int shm_map(sqlite3_file* file, int page, int page_size, int is_write, void volatile** mapped_memory)
{
	return wrap([&] { self(file)->shm_map(page, page_size, is_write, mapped_memory); });
}

int shm_lock(sqlite3_file* file, int offset, int n, int flags)
{
	return wrap([&] { self(file)->shm_lock(offset, n, flags); });
}

void shm_barrier(sqlite3_file* file)
{
	self(file)->shm_barrier();
}

int shm_unmap(sqlite3_file* file, int delete_flag)
{
	return wrap([&] { self(file)->shm_unmap(delete_flag); });
}

int fetch(sqlite3_file* file, sqlite3_int64 offset, int amount, void** buffer)
{
	return wrap([&] { self(file)->fetch(offset, amount, buffer); });
}

int unfetch(sqlite3_file* file, sqlite3_int64 offset, void* buffer)
{
	return wrap([&] { self(file)->unfetch(offset, buffer); });
}

} // namespace

File::File(const char* name, File_format format) noexcept : name{ name }, format{ format }, _methods{}
{
	_methods.iVersion               = 3;
	_methods.xClose                 = &::close;
	_methods.xRead                  = &::read;
	_methods.xWrite                 = &::write;
	_methods.xTruncate              = &::truncate;
	_methods.xSync                  = &::sync;
	_methods.xFileSize              = &::file_size;
	_methods.xLock                  = &::lock;
	_methods.xUnlock                = &::unlock;
	_methods.xCheckReservedLock     = &::check_reserved_lock;
	_methods.xFileControl           = &::file_control;
	_methods.xSectorSize            = &::sector_size; // opt
	_methods.xDeviceCharacteristics = &::device_characteristics;
	_methods.xShmLock               = &::shm_lock; // for wal
	_methods.xShmMap                = &::shm_map;
	_methods.xShmBarrier            = &::shm_barrier;
	_methods.xShmUnmap              = &::shm_unmap;
	_methods.xFetch                 = &::fetch; // if SQLITE_MAX_MMAP_SIZE > 0
	_methods.xUnfetch               = &::unfetch;
}

sqlite3_io_methods* File::methods() noexcept
{
	return &_methods;
}

int File::sector_size() const noexcept
{
	return 4096;
}
