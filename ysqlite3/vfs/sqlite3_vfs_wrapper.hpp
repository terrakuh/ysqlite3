#ifndef YSQLITE3_VFS_SQLITE3_VFS_HPP_
#define YSQLITE3_VFS_SQLITE3_VFS_HPP_

#include "../sqlite3.h"
#include "sqlite3_file_wrapper.hpp"
#include "vfs.hpp"

namespace ysqlite3 {
namespace vfs {

template<typename DerivedFile = SQLite3_file_wrapper>
class SQLite3_vfs_wrapper : public VFS
{
public:
	static_assert(std::is_base_of<SQLite3_file_wrapper, DerivedFile>::value,
	              "File must derive SQLite3_file_wrapper");

	SQLite3_vfs_wrapper(sqlite3_vfs* parent, const char* name) : VFS{ name }
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
	std::unique_ptr<File> open(const char* name, File_format format, Open_flags flags,
	                           Open_flags& output_flags) override
	{
#if PRINT_DEBUG
		printf("opening new file (%s) '%s'\n", name_of(format), name);
#endif
		std::shared_ptr<sqlite3_file> tmp_file(
		    reinterpret_cast<sqlite3_file*>(new std::uint8_t[_parent->szOsFile]{}),
		    [](sqlite3_file* x) { delete[] reinterpret_cast<std::uint8_t*>(x); });
		_assert_error(_parent->xOpen(_parent, name, tmp_file.get(), flags, &output_flags));
		std::unique_ptr<File> tmp{ new DerivedFile{ name, format, std::move(tmp_file) } };
#if PRINT_DEBUG
		printf("opening new file (%s) '%s' done\n", name_of(format), name);
#endif
		return tmp;
	}
	void delete_file(const char* name, bool sync_directory) override
	{
		_assert_error(_parent->xDelete(_parent, name, static_cast<int>(sync_directory)));
	}
	bool access(const char* name, Access_flag flag) override
	{
		int result = 0;
		_assert_error(_parent->xAccess(_parent, name, static_cast<int>(flag), &result));
		return static_cast<bool>(result);
	}
	void full_pathname(const char* input, Span<char*> output) override
	{
		_assert_error(
		    _parent->xFullPathname(_parent, input, static_cast<int>(output.size()), output.begin()));
	}
	void* dlopen(const char* filename) noexcept override
	{
		return _parent->xDlOpen(_parent, filename);
	}
	void dlerror(Span<char*> buffer) noexcept override
	{
		_parent->xDlError(_parent, static_cast<int>(buffer.size()), buffer.begin());
	}
	Dl_symbol dlsym(void* handle, const char* symbol) noexcept override
	{
		return _parent->xDlSym(_parent, handle, symbol);
	}
	void dlclose(void* handle) noexcept override
	{
		_parent->xDlClose(_parent, handle);
	}
	int random(Span<std::uint8_t*> buffer) noexcept override
	{
		return _parent->xRandomness(_parent, static_cast<int>(buffer.size()),
		                            reinterpret_cast<char*>(buffer.begin()));
	}
	Sleep_duration sleep(Sleep_duration time) noexcept override
	{
		return Sleep_duration{ _parent->xSleep(_parent, time.count()) };
	}
	Time current_time() override
	{
		sqlite3_int64 tmp = 0;
		_assert_error(_parent->xCurrentTimeInt64(_parent, &tmp));
		return Time{ tmp };
	}
	int last_error(Span<char*> buffer) noexcept override
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

	static void _assert_error(int ec)
	{
		if (ec) {
			throw std::system_error{ static_cast<SQLite3Error>(ec) };
		}
	}
};

} // namespace vfs
} // namespace ysqlite3

#endif
