#pragma once

#include "../database.hpp"
#include "../exception/sqlite3_exception.hpp"
#include "../sqlite3.h"
#include "file.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <gsl/gsl>
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

	vfs(gsl::not_null<gsl::czstring<>> name) : _name(name.get()), _vfs{}
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

	template<typename Derived>
	static typename std::enable_if<std::is_base_of<vfs, Derived>::value>::type
	    register_vfs(const std::shared_ptr<Derived>& v, bool make_default)
	{
		// static_assert(&vfs::dlopen == &Derived::dlopen, "all three dlopen, dlclose and dlsym must be overriden");

		Expects(v != nullptr);
		Expects(!v->registered());

		v->_vfs.mxPathname = static_cast<vfs*>(v.get())->max_pathname();

		auto error = sqlite3_vfs_register(&v->_vfs, make_default);

		if (error != SQLITE_OK) {
			throw;
		}

		v->_registered = v;
	}
	template<typename Derived>
	static typename std::enable_if<std::is_base_of<vfs, Derived>::value>::type unregister_vfs(gsl::not_null<Derived*> v)
	{
		auto error = sqlite3_vfs_unregister(&v->_vfs);

		if (error != SQLITE_OK) {
			throw;
		}
	}
	static sqlite3_vfs* find_vfs(gsl::czstring<> name) noexcept
	{
		return sqlite3_vfs_find(name);
	}
	bool registered() const noexcept
	{
		return _registered != nullptr;
	}
	const std::string& name() const noexcept
	{
		return _name;
	}
	gsl::not_null<sqlite3_vfs*> handle() noexcept
	{
		return &_vfs;
	}
	virtual int max_pathname() const noexcept                                             = 0;
	virtual gsl::not_null<gsl::owner<file*>> open(gsl::czstring<> name, database::open_flag_type flags,
	                                              database::open_flag_type& output_flags) = 0;
	virtual void delete_file(gsl::czstring<> name, bool sync_directory)                   = 0;
	virtual bool access(gsl::czstring<> name, access_flag flag)                           = 0;
	virtual void full_pathname(gsl::czstring<> input, gsl::string_span<> output)          = 0;
	virtual void* dlopen(gsl::czstring<> filename) noexcept
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
	virtual int set_system_call(gsl::czstring<> name, sqlite3_syscall_ptr system_call) noexcept
	{
		return SQLITE_OK;
	}
	virtual sqlite3_syscall_ptr get_system_call(gsl::czstring<> name) noexcept
	{
		return nullptr;
	}
	virtual gsl::czstring<> next_system_call(gsl::czstring<> name) noexcept
	{
		return nullptr;
	}

private:
	std::string _name;
	sqlite3_vfs _vfs;
	/** this point to itself when the vfs is registered in order to prevent deletion of this object while its
	 * registered*/
	std::shared_ptr<vfs> _registered;

	static vfs* _self(sqlite3_vfs* vfs) noexcept
	{
		return static_cast<class vfs*>(vfs->pAppData);
	}
	template<typename T>
	static int _wrap(T&& action) noexcept
	{
		try {
			action();

			return SQLITE_OK;
		} catch (const exception::sqlite3_exception& e) {
			return e.error_code();
		} catch (...) {
			return SQLITE_ERROR;
		}
	}
	static int _open(sqlite3_vfs* vfs, const char* name, sqlite3_file* file, int flags, int* out_flags) noexcept
	{
		// clear in case of an error
		std::memset(file, 0, vfs->szOsFile);

		return _wrap([=] {
			int tmp = 0;

			// open
			auto f = _self(vfs)->open(name, flags, out_flags ? *out_flags : tmp);

			file->pMethods                      = f->methods();
			*reinterpret_cast<void**>(file + 1) = f;
		});
	}
	static int _delete(sqlite3_vfs* vfs, const char* name, int sync_directory) noexcept
	{
		return _wrap([=] { _self(vfs)->delete_file(name, static_cast<bool>(sync_directory)); });
	}
	static int _access(sqlite3_vfs* vfs, const char* name, int flags, int* result) noexcept
	{
		try {
			access_flag flag;

			switch (flags) {
			case SQLITE_ACCESS_EXISTS:
			case SQLITE_ACCESS_READWRITE:
			case SQLITE_ACCESS_READ: flag = static_cast<access_flag>(flags); break;
			default: return SQLITE_IOERR_ACCESS;
			}

			*result = _self(vfs)->access(name, flag);

			return SQLITE_OK;
		} catch (...) {
			return SQLITE_IOERR_ACCESS;
		}
	}
	static int _full_pathname(sqlite3_vfs* vfs, const char* name, int size, char* buffer) noexcept
	{
		try {
			_self(vfs)->full_pathname(name, { buffer, size });

			return SQLITE_OK;
		} catch (...) {
			return SQLITE_ERROR;
		}
	}
	static void* _dlopen(sqlite3_vfs* vfs, const char* filename) noexcept
	{
		return _self(vfs)->dlopen(filename);
	}
	static void _dlerror(sqlite3_vfs* vfs, int size, char* buffer) noexcept
	{
		_self(vfs)->dlerror({ buffer, size });
	}
	static dlsym_type _dlsym(sqlite3_vfs* vfs, void* handle, const char* symbol) noexcept
	{
		if (handle) {
			return _self(vfs)->dlsym(handle, symbol);
		}

		return nullptr;
	}
	static void _dlclose(sqlite3_vfs* vfs, void* handle) noexcept
	{
		if (handle) {
			return _self(vfs)->dlclose(handle);
		}
	}
	static int _random(sqlite3_vfs* vfs, int size, char* buffer) noexcept
	{
		return _self(vfs)->random(gsl::as_writeable_bytes(gsl::make_span(buffer, size)));
	}
	static int _sleep(sqlite3_vfs* vfs, int microseconds) noexcept
	{
		return _self(vfs)->sleep(sleep_duration_type(microseconds)).count();
	}
	static int _current_time(sqlite3_vfs* vfs, double* time) noexcept
	{
		sqlite3_int64 t = 0;
		auto rc         = _current_time64(vfs, &t);

		*time = t / 86400000.0;

		return rc;
	}
	static int _last_error(sqlite3_vfs* vfs, int size, char* buffer) noexcept
	{
		return _self(vfs)->last_error({ buffer, size });
	}
	static int _current_time64(sqlite3_vfs* vfs, sqlite3_int64* time) noexcept
	{
		try {
			*time = _self(vfs)->current_time().count();

			return SQLITE_OK;
		} catch (...) {
			return SQLITE_ERROR;
		}
	}
	static int _set_system_call(sqlite3_vfs* vfs, const char* name, sqlite3_syscall_ptr system_call) noexcept
	{
		return _self(vfs)->set_system_call(name, system_call);
	}
	static sqlite3_syscall_ptr _get_system_call(sqlite3_vfs* vfs, const char* name) noexcept
	{
		return _self(vfs)->get_system_call(name);
	}
	static const char* _next_system_call(sqlite3_vfs* vfs, const char* name) noexcept
	{
		return _self(vfs)->next_system_call(name);
	}
};

} // namespace vfs
} // namespace ysqlite3