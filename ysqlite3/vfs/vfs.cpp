#include "vfs.hpp"

#include "../error.hpp"

#include <cstring>
#include <limits>
#include <random>
#include <thread>
#include <type_traits>
#include <utility>

using namespace ysqlite3::vfs;

namespace {

VFS* self(sqlite3_vfs* vfs) noexcept
{
	return static_cast<VFS*>(vfs->pAppData);
}

template<typename Action>
typename std::enable_if<std::is_void<decltype(std::declval<typename std::decay<Action>::type>()())>::value,
                        int>::type
  wrap(Action&& action) noexcept
{
	try {
		action();
		return SQLITE_OK;
	} catch (const std::system_error& e) {
		return e.code().value();
	} catch (...) {
		return SQLITE_ERROR;
	}
}

template<typename Action>
typename std::enable_if<!std::is_void<decltype(std::declval<typename std::decay<Action>::type>()())>::value,
                        int>::type
  wrap(Action&& action) noexcept
{
	try {
		return action();
	} catch (const std::system_error& e) {
		return e.code().value();
	} catch (...) {
		return SQLITE_ERROR;
	}
}

int open(sqlite3_vfs* vfs, const char* name, sqlite3_file* file, int flags, int* out_flags) noexcept
{
	// clear in case of an error
	std::memset(file, 0, vfs->szOsFile);
	return wrap([&] {
		int tmp = 0;
		File_format format;
		constexpr auto mask = SQLITE_OPEN_MAIN_DB | SQLITE_OPEN_TEMP_DB | SQLITE_OPEN_TRANSIENT_DB |
		                      SQLITE_OPEN_MAIN_JOURNAL | SQLITE_OPEN_TEMP_JOURNAL | SQLITE_OPEN_SUBJOURNAL |
		                      SQLITE_OPEN_MASTER_JOURNAL | SQLITE_OPEN_WAL;

		switch (flags & mask) {
		case SQLITE_OPEN_MAIN_DB:
		case SQLITE_OPEN_TEMP_DB:
		case SQLITE_OPEN_TRANSIENT_DB:
		case SQLITE_OPEN_MAIN_JOURNAL:
		case SQLITE_OPEN_TEMP_JOURNAL:
		case SQLITE_OPEN_SUBJOURNAL:
		case SQLITE_OPEN_MASTER_JOURNAL:
		case SQLITE_OPEN_WAL: format = static_cast<File_format>(flags & mask); break;
		default: return SQLITE_MISUSE;
		}

		// open
		auto f                              = self(vfs)->open(name, format, flags, out_flags ? *out_flags : tmp);
		file->pMethods                      = f->methods();
		*reinterpret_cast<void**>(file + 1) = f.release();
		return SQLITE_OK;
	});
}

int delete_(sqlite3_vfs* vfs, const char* name, int sync_directory) noexcept
{
	return wrap([&] { self(vfs)->delete_file(name, static_cast<bool>(sync_directory)); });
}

int access(sqlite3_vfs* vfs, const char* name, int flags, int* result) noexcept
{
	return wrap([&] {
		Access_flag flag;

		switch (flags) {
		case SQLITE_ACCESS_EXISTS:
		case SQLITE_ACCESS_READWRITE:
		case SQLITE_ACCESS_READ: flag = static_cast<Access_flag>(flags); break;
		default: return SQLITE_IOERR_ACCESS;
		}

		*result = self(vfs)->access(name, flag);
		return SQLITE_OK;
	});
}

int full_pathname(sqlite3_vfs* vfs, const char* name, int size, char* buffer) noexcept
{
	return wrap([&] { self(vfs)->full_pathname(name, { buffer, static_cast<std::size_t>(size) }); });
}

void* dlopen(sqlite3_vfs* vfs, const char* filename) noexcept
{
	return self(vfs)->dlopen(filename);
}

void dlerror(sqlite3_vfs* vfs, int size, char* buffer) noexcept
{
	return self(vfs)->dlerror({ buffer, static_cast<std::size_t>(size) });
}

Dl_symbol dlsym(sqlite3_vfs* vfs, void* handle, const char* symbol) noexcept
{
	if (handle) {
		return self(vfs)->dlsym(handle, symbol);
	}

	return nullptr;
}

void dlclose(sqlite3_vfs* vfs, void* handle) noexcept
{
	if (handle) {
		self(vfs)->dlclose(handle);
	}
}

int random_bytes(sqlite3_vfs* vfs, int size, char* buffer) noexcept
{
	return self(vfs)->random({ reinterpret_cast<std::uint8_t*>(buffer), static_cast<std::size_t>(size) });
}

int sleep(sqlite3_vfs* vfs, int microseconds) noexcept
{
	return self(vfs)->sleep(Sleep_duration{ microseconds }).count();
}

int current_time64(sqlite3_vfs* vfs, sqlite3_int64* time) noexcept
{
	return wrap([&] { *time = self(vfs)->current_time().count(); });
}

int current_time(sqlite3_vfs* vfs, double* time) noexcept
{
	sqlite3_int64 tmp = 0;
	const auto rc     = current_time64(vfs, &tmp);
	*time             = tmp / 86400000.0;
	return rc;
}

int last_error(sqlite3_vfs* vfs, int size, char* buffer) noexcept
{
	return self(vfs)->last_error({ buffer, static_cast<std::size_t>(size) });
}

int set_system_call(sqlite3_vfs* vfs, const char* name, sqlite3_syscall_ptr system_call) noexcept
{
	return self(vfs)->set_system_call(name, system_call);
}

sqlite3_syscall_ptr get_system_call(sqlite3_vfs* vfs, const char* name) noexcept
{
	return self(vfs)->get_system_call(name);
}

const char* next_system_call(sqlite3_vfs* vfs, const char* name) noexcept
{
	return self(vfs)->next_system_call(name);
}

} // namespace

VFS::VFS(const char* name) : _name{ name }, _vfs{}
{
	_vfs.iVersion = 3;
	_vfs.szOsFile = sizeof(sqlite3_file) + sizeof(void*);
	_vfs.zName    = _name.c_str();
	_vfs.pAppData = this;

	// set functions
	_vfs.xOpen   = &::open;
	_vfs.xDelete = &::delete_;
	// _vfs.xAccess       = &::access;
	_vfs.xFullPathname = &::full_pathname;
	_vfs.xDlOpen       = &::dlopen; // for extensions
	_vfs.xDlError      = &::dlerror;
	_vfs.xDlSym        = &::dlsym;
	_vfs.xDlClose      = &::dlclose;
	_vfs.xRandomness   = &::random_bytes;
	// _vfs.xSleep        = &::sleep;
	_vfs.xCurrentTime  = &::current_time;
	_vfs.xGetLastError = &::last_error;

	// version 2
	_vfs.xCurrentTimeInt64 = &::current_time64;

	// version 3
	_vfs.xSetSystemCall  = &::set_system_call;
	_vfs.xGetSystemCall  = &::get_system_call;
	_vfs.xNextSystemCall = &::next_system_call;
}

bool VFS::is_registered() const noexcept
{
	return _registered != nullptr;
}

const std::string& VFS::name() const noexcept
{
	return _name;
}

sqlite3_vfs* VFS::handle() noexcept
{
	return &_vfs;
}

void* VFS::dlopen(const char* filename) noexcept
{
	return nullptr;
}

void VFS::dlerror(std::span<char> buffer) noexcept
{
}

Dl_symbol VFS::dlsym(void* handle, const char* symbol) noexcept
{
	return nullptr;
}

void VFS::dlclose(void* handle) noexcept
{
}

int VFS::random(std::span<std::uint8_t> buffer) noexcept
{
	std::default_random_engine engine;
	std::uniform_int_distribution<int> distributor(std::numeric_limits<std::uint8_t>::min(),
	                                               std::numeric_limits<std::uint8_t>::max());
	for (auto& i : buffer) {
		i = static_cast<std::uint8_t>(distributor(engine));
	}
	return static_cast<int>(buffer.size());
}

Sleep_duration VFS::sleep(Sleep_duration time) noexcept
{
	std::this_thread::sleep_for(time);
	return time;
}

Time VFS::current_time()
{
	using namespace std::chrono;
	return duration_cast<Time>(system_clock::now().time_since_epoch() +
	                           std::chrono::milliseconds{ 210866760000000 });
}

int VFS::last_error(std::span<char> buffer) noexcept
{
	if (!buffer.empty()) {
		*buffer.begin() = 0;
	}
	return 0;
}

int VFS::set_system_call(const char* name, sqlite3_syscall_ptr system_call) noexcept
{
	return SQLITE_OK;
}

sqlite3_syscall_ptr VFS::get_system_call(const char* name) noexcept
{
	return nullptr;
}

const char* VFS::next_system_call(const char* name) noexcept
{
	return nullptr;
}
