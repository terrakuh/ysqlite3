#ifndef YSQLITE3_VFS_VFS_HPP_
#define YSQLITE3_VFS_VFS_HPP_

#include "../database.hpp"
#include "../error.hpp
#include "../sqlite3.h"
#include "file.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

namespace ysqlite3 {
namespace vfs {

class vfs
{
public:
	typedef void (*dlsym_type)();
	typedef std::chrono::duration<int, std::micro> sleep_duration_type;
	typedef std::chrono::duration<sqlite3_int64, std::milli> time_type;

	enum class access_flag
	{
		exists    = SQLITE_ACCESS_EXISTS,
		readwrite = SQLITE_ACCESS_READWRITE,
		read      = SQLITE_ACCESS_READ
	};

	/**
	 Creates a VFS and initializes its values. After initialization a VFS must registered, see
	 vfs::register_vfs().

	 @pre name cannot be empty

	 @param name the name of this VFS
	*/
	vfs(const char* name) : _name(name.get()), _vfs{}
	{
		Expects(!_name.empty());

		_vfs.iVersion = 3;
		_vfs.szOsFile = sizeof(sqlite3_file) + sizeof(void*);
		_vfs.zName    = _name.c_str();
		_vfs.pAppData = this;

		// set functions
		_vfs.xOpen         = _open;
		_vfs.xDelete       = _delete;
		_vfs.xAccess       = _access;
		_vfs.xFullPathname = _full_pathname;
		_vfs.xDlOpen       = _dlopen; // for extensions
		_vfs.xDlError      = _dlerror;
		_vfs.xDlSym        = _dlsym;
		_vfs.xDlClose      = _dlclose;
		_vfs.xRandomness   = _random;
		_vfs.xSleep        = _sleep;
		_vfs.xCurrentTime  = _current_time;
		_vfs.xGetLastError = _last_error;

		// version 2
		_vfs.xCurrentTimeInt64 = _current_time64;

		// version 3
		_vfs.xSetSystemCall  = _set_system_call;
		_vfs.xGetSystemCall  = _get_system_call;
		_vfs.xNextSystemCall = _next_system_call;
	}
	vfs(const vfs& copy) = delete;
	vfs(vfs&& move)      = delete;
	virtual ~vfs()       = default;
	/**
	 *Checks whether this VFS has been registered.
	 *
	 *@return `true` if this VFS was registered, otherwise `false`
	 */
	bool is_registered() const noexcept
	{
		return _registered != nullptr;
	}
	/**
	 * Returns the name of this VFS.
	 *
	 * @return the name of this VFS
	 */
	const std::string& name() const noexcept
	{
		return _name;
	}
	/**
	 * Returns the SQLite VFS handle.
	 *
	 * @return the handle
	 */
	sqlite3_vfs* handle() noexcept
	{
		return &_vfs;
	}
	/**
	 Returns the maximum allowed pathname length for this VFS.

	 @return the max pathname length
	*/
	virtual int max_pathname() const noexcept = 0;
	/**
	 Opens the specified with the flags.

	 @param name the file path
	 @param format the format of the file
	 @param flags the open flags; see database::open_flag_type
	 @param output_flags[out] the applied flags
	 @returns the file object
	 @throws may throw anything
	 @see for more information look at the official [SQLite page](https://www.sqlite.org/c3ref/vfs.html)
	*/
	virtual gsl::not_null<gsl::owner<file*>> open(gsl::czstring<> name, file::format format,
	                                              database::open_flag_type flags,
	                                              database::open_flag_type& output_flags) = 0;
	virtual void delete_file(gsl::czstring<> name, bool sync_directory)                   = 0;
	virtual bool access(gsl::czstring<> name, access_flag flag)                           = 0;
	virtual void full_pathname(gsl::czstring<> input, gsl::string_span<> output)          = 0;
	virtual void* dlopen(const char* filename) noexcept
	{
		return nullptr;
	}
	virtual void dlerror(gsl::string_span<> buffer) noexcept
	{}
	virtual dlsym_type dlsym(gsl::not_null<void*> handle, gsl::czstring<> symbol) noexcept
	{
		return nullptr;
	}
	virtual void dlclose(gsl::not_null<void*> handle) noexcept
	{}
	virtual int random(gsl::span<gsl::byte> buffer) noexcept
	{
		typedef typename std::underlying_type<gsl::byte>::type byte_type;

		std::default_random_engine engine;
		std::uniform_int_distribution<int> distributor(std::numeric_limits<byte_type>::min(),
		                                               std::numeric_limits<byte_type>::max());

		for (auto& i : buffer) {
			i = static_cast<gsl::byte>(distributor(engine));
		}

		return gsl::narrow_cast<int>(buffer.size());
	}
	virtual sleep_duration_type sleep(sleep_duration_type time) noexcept
	{
		std::this_thread::sleep_for(time);

		return time;
	}
	virtual time_type current_time()
	{
		using namespace std::chrono_literals;
		using namespace std::chrono;

		return duration_cast<time_type>(system_clock::now().time_since_epoch() + 210866760000000ms);
	}
	virtual int last_error(gsl::string_span<> buffer) noexcept
	{
		return 0;
	}
	virtual int set_system_call(const char* name, sqlite3_syscall_ptr system_call) noexcept
	{
		return SQLITE_OK;
	}
	virtual sqlite3_syscall_ptr get_system_call(const char* name) noexcept
	{
		return nullptr;
	}
	virtual const char* next_system_call(const char* name) noexcept
	{
		return nullptr;
	}
	template<typename Derived>
	friend typename std::enable_if<std::is_base_of<vfs, Derived>::value>::type
	    register_vfs(std::shared_ptr<Derived> v, bool make_default)
	{
		// static_assert(&vfs::dlopen == &Derived::dlopen, "all three dlopen, dlclose and dlsym must be
		// overriden");

		Expects(v != nullptr);
		Expects(!v->registered());

		v->_vfs.mxPathname = v->max_pathname();

		auto error = sqlite3_vfs_register(&v->_vfs, make_default);

		if (error != SQLITE_OK) {
			throw;
		}

		v->_registered = v;
	}
	template<typename Derived>
	friend typename std::enable_if<std::is_base_of<vfs, Derived>::value>::type
	    unregister_vfs(gsl::not_null<Derived*> v)
	{
		if (const auto err = sqlite3_vfs_unregister(&v->_vfs)) {
			throw std::system_error{ static_cast<ysqlite3::sqlite3::errc>(err) };
		}
	}

private:
	std::string _name;
	sqlite3_vfs _vfs;
	/** this point to itself when the vfs is registered in order to prevent deletion of this object while its
	 * registered*/
	std::shared_ptr<vfs> _registered;
};

inline sqlite3_vfs* find_vfs(const char* name) noexcept
{
	return sqlite3_vfs_find(name);
}

} // namespace vfs
} // namespace ysqlite3

#endif
