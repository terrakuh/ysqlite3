#ifndef YSQLITE3_VFS_VFS_HPP_
#define YSQLITE3_VFS_VFS_HPP_

#include "../database.hpp"
#include "../span.hpp"
#include "../sqlite3.h"
#include "file.hpp"

#include <chrono>
#include <memory>

namespace ysqlite3 {
namespace vfs {

typedef void (*dlsym_type)();
typedef std::chrono::duration<int, std::micro> sleep_duration_type;
typedef std::chrono::duration<sqlite3_int64, std::milli> time_type;

enum class access_flag
{
	exists    = SQLITE_ACCESS_EXISTS,
	readwrite = SQLITE_ACCESS_READWRITE,
	read      = SQLITE_ACCESS_READ
};

class vfs
{
public:
	/**
	 * Creates a VFS and initializes its values. After initialization a VFS must registered, see
	 * vfs::register_vfs().
	 *
	 * @pre name cannot be empty
	 *
	 * @param name the name of this VFS
	 */
	vfs(const char* name);
	vfs(const vfs& copy) = delete;
	vfs(vfs&& move)      = delete;
	virtual ~vfs()       = default;
	/**
	 * Checks whether this VFS has been registered.
	 *
	 * @return `true` if this VFS was registered, otherwise `false`
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
	 * Returns the maximum allowed pathname length for this VFS.
	 *
	 * @return the max pathname length
	 */
	virtual int max_pathname() const noexcept = 0;
	/**
	 * Opens the specified with the flags.
	 *
	 * @param name the file path
	 * @param format the format of the file
	 * @param flags the open flags; see open_flag_type
	 * @param[out] output_flags the applied flags
	 * @return the file object
	 * @see for more information look at the official [SQLite page](https://www.sqlite.org/c3ref/vfs.html)
	 */
	virtual std::unique_ptr<file> open(const char* name, file_format format, open_flag_type flags,
	                                   open_flag_type& output_flags)  = 0;
	virtual void delete_file(const char* name, bool sync_directory)   = 0;
	virtual bool access(const char* name, access_flag flag)           = 0;
	virtual void full_pathname(const char* input, span<char*> output) = 0;
	virtual void* dlopen(const char* filename) noexcept
	{
		return nullptr;
	}
	virtual void dlerror(span<char*> buffer) noexcept
	{}
	virtual dlsym_type dlsym(void* handle, const char* symbol) noexcept
	{
		return nullptr;
	}
	virtual void dlclose(void* handle) noexcept
	{}
	virtual int random(span<std::uint8_t*> buffer) noexcept;
	virtual sleep_duration_type sleep(sleep_duration_type time) noexcept;
	virtual time_type current_time();
	virtual int last_error(span<char*> buffer) noexcept
	{
		if (!buffer.empty()) {
			*buffer.begin() = 0;
		}

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
	    register_vfs(std::shared_ptr<Derived> v, bool make_default);
	template<typename Derived>
	friend typename std::enable_if<std::is_base_of<vfs, Derived>::value>::type unregister_vfs(Derived& v)
	{
		if (const auto ec = sqlite3_vfs_unregister(&v._vfs)) {
			throw std::system_error{ static_cast<sqlite3_errc>(ec) };
		}
	}

private:
	std::string _name;
	sqlite3_vfs _vfs;
	/** this point to itself when the vfs is registered in order to prevent deletion of this object while its
	 * registered*/
	std::shared_ptr<vfs> _registered;
};

template<typename Derived>
inline typename std::enable_if<std::is_base_of<vfs, Derived>::value>::type
    register_vfs(std::shared_ptr<Derived> vfs, bool make_default)
{
	// static_assert(&vfs::dlopen == &Derived::dlopen, "all three dlopen, dlclose and dlsym must be
	// overriden");

	if (!vfs) {
		throw std::system_error{ errc::bad_arguments };
	} else if (vfs->is_registered()) {
		throw std::system_error{ errc::vfs_already_registered };
	}

	vfs->_vfs.mxPathname = vfs->max_pathname();

	if (const auto ec = sqlite3_vfs_register(&vfs->_vfs, make_default)) {
		throw std::system_error{ static_cast<sqlite3_errc>(ec) };
	}

	vfs->_registered = vfs;
}

inline sqlite3_vfs* find_vfs(const char* name) noexcept
{
	return sqlite3_vfs_find(name);
}

} // namespace vfs
} // namespace ysqlite3

#endif
