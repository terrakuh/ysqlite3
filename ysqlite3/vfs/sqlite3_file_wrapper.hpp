#ifndef YSQLITE3_VFS_SQLITE3_FILE_HPP_
#define YSQLITE3_VFS_SQLITE3_FILE_HPP_

#include "../exception/sqlite3_exception.hpp"
#include "../sqlite3.h"
#include "file.hpp"

#include <gsl/gsl>
#include <memory>

namespace ysqlite3 {
namespace vfs {

class sqlite3_file_wrapper : public file
{
public:
	sqlite3_file_wrapper(gsl::not_null<gsl::shared_ptr<sqlite3_file>> parent) noexcept : _parent(std::move(parent))
	{}

	virtual void close() override
	{
		auto _ = gsl::finally([this] { _parent = nullptr; });

		_check_error(_parent->pMethods->xClose(_parent.get()));
	}
	virtual void read(gsl::span<gsl::byte> buffer, sqlite3_int64 offset) override
	{
		_check_error(
		    _parent->pMethods->xRead(_parent.get(), buffer.data(), gsl::narrow_cast<int>(buffer.size()), offset));
	}
	virtual void write(gsl::span<const gsl::byte> buffer, sqlite3_int64 offset) override
	{
		_check_error(
		    _parent->pMethods->xWrite(_parent.get(), buffer.data(), gsl::narrow_cast<int>(buffer.size()), offset));
	}
	virtual void truncate(sqlite3_int64 size) override
	{
		_check_error(_parent->pMethods->xTruncate(_parent.get(), size));
	}
	virtual void sync(sync_flag flag) override
	{
		_check_error(_parent->pMethods->xSync(_parent.get(), static_cast<int>(flag)));
	}
	virtual sqlite3_int64 file_size() const override
	{
		sqlite3_int64 size = 0;

		_check_error(_parent->pMethods->xFileSize(_parent.get(), &size));

		return size;
	}
	virtual void lock(lock_flag flag) override
	{
		_check_error(_parent->pMethods->xLock(_parent.get(), static_cast<int>(flag)));
	}
	virtual void unlock(lock_flag flag) override
	{
		_check_error(_parent->pMethods->xUnlock(_parent.get(), static_cast<int>(flag)));
	}
	virtual bool has_reserved_lock() const override
	{
		int result = 0;

		_check_error(_parent->pMethods->xCheckReservedLock(_parent.get(), &result));

		return static_cast<bool>(result);
	}
	virtual void file_control(file_cntl operation, void* arg) override
	{
		_check_error(_parent->pMethods->xFileControl(_parent.get(), operation, arg));
	}
	virtual int sector_size() const noexcept override
	{
		return _parent->pMethods->xSectorSize ? _parent->pMethods->xSectorSize(_parent.get()) : file::sector_size();
	}
	virtual int device_characteristics() const noexcept override
	{
		return _parent->pMethods->xDeviceCharacteristics(_parent.get());
	}

private:
	std::shared_ptr<sqlite3_file> _parent;

	static void _check_error(int code)
	{
		if (code != SQLITE_OK) {
			YSQLITE_THROW(exception::sqlite3_exception, code, "operation failed");
		}
	}
};

} // namespace vfs
} // namespace ysqlite3

#endif // !YSQLITE3_VFS_SQLITE3_FILE_HPP_
