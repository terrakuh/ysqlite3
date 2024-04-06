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

typedef void (*Dl_symbol)();
typedef std::chrono::duration<int, std::micro> Sleep_duration;
typedef std::chrono::duration<sqlite3_int64, std::milli> Time;

enum class Access_flag
{
	exists    = SQLITE_ACCESS_EXISTS,
	readwrite = SQLITE_ACCESS_READWRITE,
	read      = SQLITE_ACCESS_READ
};

class VFS
{
public:
	/**
	 * Creates a VFS and initializes its values. After initialization a VFS must registered, see
	 * VFS::register_vfs().
	 *
	 * @pre name cannot be empty
	 *
	 * @param name the name of this VFS
	 */
	VFS(const char* name);
	VFS(const VFS& copy) = delete;
	VFS(VFS&& move)      = delete;
	virtual ~VFS()       = default;
	/// Checks whether this VFS has been registered.
	bool is_registered() const noexcept;
	/// Returns the name of this VFS.
	const std::string& name() const noexcept;
	/// Returns the SQLite VFS handle.
	sqlite3_vfs* handle() noexcept;
	/// Returns the maximum allowed pathname length for this VFS.
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
	virtual std::unique_ptr<File> open(const char* name, File_format format, Open_flags flags,
	                                   Open_flags& output_flags)      = 0;
	virtual void delete_file(const char* name, bool sync_directory)   = 0;
	virtual bool access(const char* name, Access_flag flag)           = 0;
	virtual void full_pathname(const char* input, Span<char*> output) = 0;
	virtual void* dlopen(const char* filename) noexcept;
	virtual void dlerror(Span<char*> buffer) noexcept;
	virtual Dl_symbol dlsym(void* handle, const char* symbol) noexcept;
	virtual void dlclose(void* handle) noexcept;
	virtual int random(Span<std::uint8_t*> buffer) noexcept;
	virtual Sleep_duration sleep(Sleep_duration time) noexcept;
	virtual Time current_time();
	virtual int last_error(Span<char*> buffer) noexcept;
	virtual int set_system_call(const char* name, sqlite3_syscall_ptr system_call) noexcept;
	virtual sqlite3_syscall_ptr get_system_call(const char* name) noexcept;
	virtual const char* next_system_call(const char* name) noexcept;
	template<typename Derived>
	friend typename std::enable_if<std::is_base_of<VFS, Derived>::value>::type
	    register_vfs(std::shared_ptr<Derived> vfs, bool make_default);
	template<typename Derived>
	friend typename std::enable_if<std::is_base_of<VFS, Derived>::value>::type unregister_vfs(Derived& vfs);

private:
	std::string _name;
	sqlite3_vfs _vfs;
	/// Points to itself if the VFS is registered in order to prevent deletion of this object while its
	/// registered.
	std::shared_ptr<VFS> _registered;
};

template<typename Derived>
inline typename std::enable_if<std::is_base_of<VFS, Derived>::value>::type
    register_vfs(std::shared_ptr<Derived> vfs, bool make_default)
{
	// static_assert(&vfs::dlopen == &Derived::dlopen, "all three dlopen, dlclose and dlsym must be
	// overriden");

	if (!vfs) {
		throw std::system_error{ Error::bad_arguments };
	} else if (vfs->is_registered()) {
		throw std::system_error{ Error::vfs_already_registered };
	}

	vfs->_vfs.mxPathname = vfs->max_pathname();
	if (const auto ec = sqlite3_vfs_register(&vfs->_vfs, make_default)) {
		throw std::system_error{ static_cast<SQLite3Error>(ec) };
	}
	vfs->_registered = vfs;
}
template<typename Derived>
inline typename std::enable_if<std::is_base_of<VFS, Derived>::value>::type unregister_vfs(Derived& vfs)
{
	if (const auto ec = sqlite3_vfs_unregister(&vfs._vfs)) {
		throw std::system_error{ static_cast<SQLite3Error>(ec) };
	}
}

inline sqlite3_vfs* find_vfs(const char* name) noexcept
{
	return sqlite3_vfs_find(name);
}

} // namespace vfs
} // namespace ysqlite3

#endif
