#include "sqlite3_file_wrapper.hpp"

#include "../error.hpp"
#include "../finally.hpp"

using namespace ysqlite3::vfs;

inline void assert_error(int ec)
{
	if (ec) {
		throw std::system_error{ static_cast<ysqlite3::SQLite3Error>(ec) };
	}
}

SQLite3_file_wrapper::SQLite3_file_wrapper(const char* name, File_format format,
                                           std::shared_ptr<sqlite3_file> parent) noexcept
    : File{ name, format }, _parent{ std::move(parent) }
{
}

void SQLite3_file_wrapper::close()
{
#if PRINT_DEBUG
	printf("closing file: %s\n", name_of(format));
#endif
	const auto _ = finally([this] { _parent = nullptr; });
	assert_error(_parent->pMethods->xClose(_parent.get()));
}

void SQLite3_file_wrapper::read(std::span<std::byte> buffer, sqlite3_int64 offset)
{
	assert_error(
	  _parent->pMethods->xRead(_parent.get(), buffer.data(), static_cast<int>(buffer.size()), offset));
}

void SQLite3_file_wrapper::write(std::span<const std::byte> buffer, sqlite3_int64 offset)
{
	assert_error(
	  _parent->pMethods->xWrite(_parent.get(), buffer.data(), static_cast<int>(buffer.size()), offset));
}

void SQLite3_file_wrapper::truncate(sqlite3_int64 size)
{
	assert_error(_parent->pMethods->xTruncate(_parent.get(), size));
}

void SQLite3_file_wrapper::sync(Sync_flag flag)
{
	assert_error(_parent->pMethods->xSync(_parent.get(), static_cast<int>(flag)));
}

sqlite3_int64 SQLite3_file_wrapper::file_size() const
{
	sqlite3_int64 size = 0;
	assert_error(_parent->pMethods->xFileSize(_parent.get(), &size));
	return size;
}

void SQLite3_file_wrapper::lock(Lock_flag flag)
{
	assert_error(_parent->pMethods->xLock(_parent.get(), static_cast<int>(flag)));
}

void SQLite3_file_wrapper::unlock(Lock_flag flag)
{
	assert_error(_parent->pMethods->xUnlock(_parent.get(), static_cast<int>(flag)));
}

bool SQLite3_file_wrapper::has_reserved_lock() const
{
	int result = 0;
	assert_error(_parent->pMethods->xCheckReservedLock(_parent.get(), &result));
	return static_cast<bool>(result);
}

void SQLite3_file_wrapper::file_control(File_control operation, void* arg)
{
	assert_error(_parent->pMethods->xFileControl(_parent.get(), static_cast<int>(operation), arg));
}

int SQLite3_file_wrapper::sector_size() const noexcept
{
	return _parent->pMethods->xSectorSize ? _parent->pMethods->xSectorSize(_parent.get()) : File::sector_size();
}

int SQLite3_file_wrapper::device_characteristics() const noexcept
{
	return _parent->pMethods->xDeviceCharacteristics(_parent.get());
}

void SQLite3_file_wrapper::shm_map(int page, int page_size, bool is_write, void volatile** mapped_memory)
{
	assert_error(_parent->pMethods->xShmMap(_parent.get(), page, page_size, is_write, mapped_memory));
}

void SQLite3_file_wrapper::shm_lock(int offset, int n, int flags)
{
	assert_error(_parent->pMethods->xShmLock(_parent.get(), offset, n, flags));
}

void SQLite3_file_wrapper::shm_barrier() noexcept
{
	_parent->pMethods->xShmBarrier(_parent.get());
}

void SQLite3_file_wrapper::shm_unmap(int delete_flag)
{
	assert_error(_parent->pMethods->xShmUnmap(_parent.get(), delete_flag));
}

void SQLite3_file_wrapper::fetch(sqlite3_int64 offset, int amount, void** buffer)
{
	assert_error(_parent->pMethods->xFetch(_parent.get(), offset, amount, buffer));
}

void SQLite3_file_wrapper::unfetch(sqlite3_int64 offset, void* buffer)
{
	assert_error(_parent->pMethods->xUnfetch(_parent.get(), offset, buffer));
}
