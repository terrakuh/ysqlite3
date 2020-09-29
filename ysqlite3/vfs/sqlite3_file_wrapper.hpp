#ifndef YSQLITE3_VFS_SQLITE3_FILE_HPP_
#define YSQLITE3_VFS_SQLITE3_FILE_HPP_

#include "../error.hpp"
#include "../finally.hpp"
#include "../sqlite3.h"
#include "file.hpp"

#include <memory>

namespace ysqlite3 {
namespace vfs {

class sqlite3_file_wrapper : public file
{
public:
	sqlite3_file_wrapper(file_format format, std::shared_ptr<sqlite3_file> parent) noexcept
	    : file{ format }, _parent{ std::move(parent) }
	{}
	void close() override
	{
		const auto _ = finally([this] { _parent = nullptr; });
		_check_error(_parent->pMethods->xClose(_parent.get()));
	}
	void read(span<std::uint8_t*> buffer, sqlite3_int64 offset) override
	{
		_check_error(
		    _parent->pMethods->xRead(_parent.get(), buffer.begin(), static_cast<int>(buffer.size()), offset));
	}
	void write(span<const std::uint8_t*> buffer, sqlite3_int64 offset) override
	{
		_check_error(_parent->pMethods->xWrite(_parent.get(), buffer.begin(), static_cast<int>(buffer.size()),
		                                       offset));
	}
	void truncate(sqlite3_int64 size) override
	{
		_check_error(_parent->pMethods->xTruncate(_parent.get(), size));
	}
	void sync(sync_flag flag) override
	{
		_check_error(_parent->pMethods->xSync(_parent.get(), static_cast<int>(flag)));
	}
	sqlite3_int64 file_size() const override
	{
		sqlite3_int64 size = 0;
		_check_error(_parent->pMethods->xFileSize(_parent.get(), &size));
		return size;
	}
	void lock(lock_flag flag) override
	{
		_check_error(_parent->pMethods->xLock(_parent.get(), static_cast<int>(flag)));
	}
	void unlock(lock_flag flag) override
	{
		_check_error(_parent->pMethods->xUnlock(_parent.get(), static_cast<int>(flag)));
	}
	bool has_reserved_lock() const override
	{
		int result = 0;

		_check_error(_parent->pMethods->xCheckReservedLock(_parent.get(), &result));

		return static_cast<bool>(result);
	}
	void file_control(file_cntl operation, void* arg) override
	{
		_check_error(_parent->pMethods->xFileControl(_parent.get(), static_cast<int>(operation), arg));
	}
	int sector_size() const noexcept override
	{
		return _parent->pMethods->xSectorSize ? _parent->pMethods->xSectorSize(_parent.get())
		                                      : file::sector_size();
	}
	int device_characteristics() const noexcept override
	{
		return _parent->pMethods->xDeviceCharacteristics(_parent.get());
	}

private:
	std::shared_ptr<sqlite3_file> _parent;

	static void _check_error(int ec)
	{
		if (ec) {
			throw std::system_error{ static_cast<sqlite3_errc>(ec) };
		}
	}
};

} // namespace vfs
} // namespace ysqlite3

#endif // !YSQLITE3_VFS_SQLITE3_FILE_HPP_
