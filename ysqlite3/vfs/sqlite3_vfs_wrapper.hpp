#ifndef YSQLITE3_VFS_SQLITE3_VFS_HPP_
#define YSQLITE3_VFS_SQLITE3_VFS_HPP_

#include "../sqlite3.h"
#include "sqlite3_file_wrapper.hpp"
#include "vfs.hpp"

#include <gsl/gsl>

namespace ysqlite3 {
namespace vfs {

template<typename File = sqlite3_file_wrapper>
class sqlite3_vfs_wrapper : public vfs<File>
{
public:
	sqlite3_vfs_wrapper(sqlite3_vfs* parent, gsl::not_null<gsl::czstring<>> name) : vfs(name)
	{
		_parent = parent;
	}

	sqlite3_vfs* parent() noexcept
	{
		return _parent;
	}
	virtual int max_pathname() const noexcept override
	{
		return _parent->mxPathname;
	}
	virtual gsl::not_null<gsl::owner<file*>> open(gsl::czstring<> name, database::open_flag_type flags,
	                                              database::open_flag_type& output_flags) override
	{
		std::shared_ptr<sqlite3_file> tmp_file(reinterpret_cast<sqlite3_file*>(new gsl::byte[_parent->szOsFile]{}),
		                                       [](sqlite3_file* x) { delete[] reinterpret_cast<gsl::byte*>(x); });

		_check_error(_parent->xOpen(_parent, name, tmp_file.get(), flags, &output_flags));

		// create vfs file
		return new sqlite3_file_wrapper(std::move(tmp_file));
	}
	virtual void delete_file(gsl::czstring<> name, bool sync_directory) override
	{
		_check_error(_parent->xDelete(_parent, name, static_cast<int>(sync_directory)));
	}
	virtual bool access(gsl::czstring<> name, access_flag flag) override
	{
		int result = 0;

		_check_error(_parent->xAccess(_parent, name, static_cast<int>(flag), &result));

		return static_cast<bool>(result);
	}
	virtual void full_pathname(gsl::czstring<> input, gsl::string_span<> output) override
	{
		_check_error(_parent->xFullPathname(_parent, input, gsl::narrow_cast<int>(output.size()), output.data()));
	}
	virtual void* dlopen(gsl::czstring<> filename) noexcept override
	{
		return _parent->xDlOpen(_parent, filename);
	}
	virtual void dlerror(gsl::string_span<> buffer) noexcept override
	{
		_parent->xDlError(_parent, gsl::narrow_cast<int>(buffer.size()), buffer.data());
	}
	virtual dlsym_type dlsym(gsl::not_null<void*> handle, gsl::czstring<> symbol) noexcept override
	{
		return _parent->xDlSym(_parent, handle, symbol);
	}
	virtual void dlclose(gsl::not_null<void*> handle) noexcept override
	{
		_parent->xDlClose(_parent, handle);
	}
	virtual int random(gsl::span<gsl::byte> buffer) noexcept override
	{
		return _parent->xRandomness(_parent, gsl::narrow_cast<int>(buffer.size()),
		                            reinterpret_cast<char*>(buffer.data()));
	}
	virtual sleep_duration_type sleep(sleep_duration_type time) noexcept override
	{
		return sleep_duration_type(_parent->xSleep(_parent, time.count()));
	}
	virtual time_type current_time() override
	{
		sqlite3_int64 t = 0;

		_check_error(_parent->xCurrentTimeInt64(_parent, &t));

		return time_type(t);
	}
	virtual int last_error(gsl::string_span<> buffer) noexcept override
	{
		return _parent->xGetLastError(_parent, gsl::narrow_cast<int>(buffer.size()), buffer.data());
	}
	virtual int set_system_call(gsl::czstring<> name, sqlite3_syscall_ptr system_call) noexcept override
	{
		return _parent->xSetSystemCall(_parent, name, system_call);
	}
	virtual sqlite3_syscall_ptr get_system_call(gsl::czstring<> name) noexcept override
	{
		return _parent->xGetSystemCall(_parent, name);
	}
	virtual gsl::czstring<> next_system_call(gsl::czstring<> name) noexcept override
	{
		return _parent->xNextSystemCall(_parent, name);
	}

private:
	sqlite3_vfs* _parent;

	static void _check_error(int code)
	{
		if (code != SQLITE_OK) {
			throw;
		}
	}
};

} // namespace vfs
} // namespace ysqlite3

#endif // !YSQLITE3_VFS_SQLITE3_VFS_HPP_
