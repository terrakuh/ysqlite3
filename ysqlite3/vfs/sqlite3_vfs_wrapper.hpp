#ifndef YSQLITE3_VFS_SQLITE3_VFS_HPP_
#define YSQLITE3_VFS_SQLITE3_VFS_HPP_

#include "../sqlite3.h"
#include "sqlite3_file_wrapper.hpp"
#include "vfs.hpp"

namespace ysqlite3 {
namespace vfs {

template<typename File = sqlite3_file_wrapper>
class sqlite3_vfs_wrapper : public vfs
{
public:
	static_assert(std::is_base_of<sqlite3_file_wrapper, File>::value,
	              "file must be derive sqlite3_file_wrapper");

	typedef File file_type;

	sqlite3_vfs_wrapper(sqlite3_vfs* parent, const char* name) : vfs{ name }
	{
		_parent = parent;
	}
	sqlite3_vfs* parent() noexcept
	{
		return _parent;
	}
	int max_pathname() const noexcept override
	{
		return _parent->mxPathname;
	}
	std::unique_ptr<file> open(const char* name, file_format format, open_flag_type flags,
	                           open_flag_type& output_flags) override
	{
		std::shared_ptr<sqlite3_file> tmp_file(
		    reinterpret_cast<sqlite3_file*>(new std::uint8_t[_parent->szOsFile]{}),
		    [](sqlite3_file* x) { delete[] reinterpret_cast<std::uint8_t*>(x); });
		_check_error(_parent->xOpen(_parent, name, tmp_file.get(), flags, &output_flags));
		return std::unique_ptr<file>{ new File{ format, std::move(tmp_file) } };
	}
	void delete_file(const char* name, bool sync_directory) override
	{
		_check_error(_parent->xDelete(_parent, name, static_cast<int>(sync_directory)));
	}
	bool access(const char* name, access_flag flag) override
	{
		int result = 0;
		_check_error(_parent->xAccess(_parent, name, static_cast<int>(flag), &result));
		return static_cast<bool>(result);
	}
	void full_pathname(const char* input, span<char*> output) override
	{
		_check_error(_parent->xFullPathname(_parent, input, static_cast<int>(output.size()), output.begin()));
	}
	void* dlopen(const char* filename) noexcept override
	{
		return _parent->xDlOpen(_parent, filename);
	}
	void dlerror(span<char*> buffer) noexcept override
	{
		_parent->xDlError(_parent, static_cast<int>(buffer.size()), buffer.begin());
	}
	dlsym_type dlsym(void* handle, const char* symbol) noexcept override
	{
		return _parent->xDlSym(_parent, handle, symbol);
	}
	void dlclose(void* handle) noexcept override
	{
		_parent->xDlClose(_parent, handle);
	}
	int random(span<std::uint8_t*> buffer) noexcept override
	{
		return _parent->xRandomness(_parent, static_cast<int>(buffer.size()),
		                            reinterpret_cast<char*>(buffer.begin()));
	}
	sleep_duration_type sleep(sleep_duration_type time) noexcept override
	{
		return sleep_duration_type{ _parent->xSleep(_parent, time.count()) };
	}
	time_type current_time() override
	{
		sqlite3_int64 tmp = 0;
		_check_error(_parent->xCurrentTimeInt64(_parent, &tmp));
		return time_type{ tmp };
	}
	int last_error(span<char*> buffer) noexcept override
	{
		return _parent->xGetLastError(_parent, static_cast<int>(buffer.size()), buffer.begin());
	}
	int set_system_call(const char* name, sqlite3_syscall_ptr system_call) noexcept override
	{
		return _parent->xSetSystemCall(_parent, name, system_call);
	}
	sqlite3_syscall_ptr get_system_call(const char* name) noexcept override
	{
		return _parent->xGetSystemCall(_parent, name);
	}
	const char* next_system_call(const char* name) noexcept override
	{
		return _parent->xNextSystemCall(_parent, name);
	}

private:
	sqlite3_vfs* _parent;

	static void _check_error(int ec)
	{
		if (ec) {
			throw std::system_error{ static_cast<sqlite3_errc>(ec) };
		}
	}
};

} // namespace vfs
} // namespace ysqlite3

#endif // !YSQLITE3_VFS_SQLITE3_VFS_HPP_
