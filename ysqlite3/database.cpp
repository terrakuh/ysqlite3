#include "database.hpp"

#include "error.hpp"
#include "finally.hpp"

#include <utility>

namespace ysqlite3 {

Database::Database(const char* file)
{
	open(file);
}

Database::Database(Database&& move) noexcept
{
	std::swap(_database, move._database);
}

Database::~Database() noexcept
{
	close(true);
}

std::uint8_t Database::set_reserved_size(std::uint8_t size, bool vacuum)
{
	if (!is_open()) {
		throw std::system_error{ Error::database_is_closed };
	}

	int n  = size;
	int ec = sqlite3_file_control(_database, nullptr, SQLITE_FCNTL_RESERVE_BYTES, &n);
	if (!ec && vacuum && n != static_cast<int>(size)) {
		ec = sqlite3_exec(_database, "VACUUM", nullptr, nullptr, nullptr);
		// reset
		if (ec) {
			sqlite3_file_control(_database, nullptr, SQLITE_FCNTL_RESERVE_BYTES, &n);
		}
	}
	if (ec) {
		throw std::system_error{ static_cast<SQLite3Error>(ec) };
	}
	return static_cast<std::uint8_t>(n);
}

void Database::set_journal_mode(JournalMode mode)
{
	switch (mode) {
	case JournalMode::delete_: execute("PRAGMA journal_mode=DELETE;"); break;
	case JournalMode::truncate: execute("PRAGMA journal_mode=TRUNCATE;"); break;
	case JournalMode::persist: execute("PRAGMA journal_mode=PERSIST;"); break;
	case JournalMode::memory: execute("PRAGMA journal_mode=MEMORY;"); break;
	case JournalMode::wal: execute("PRAGMA journal_mode=WAL;"); break;
	case JournalMode::off: execute("PRAGMA journal_mode=OFF;"); break;
	}
}

void Database::enable_foreign_keys(bool enable)
{
	execute(enable ? "PRAGMA foreign_keys=ON" : "PRAGMA foreign_keys=OFF");
}

void Database::close(bool force)
{
	if (_database) {
		if (force) {
			sqlite3_close_v2(_database);
		} else {
			if (const auto ec = sqlite3_close(_database)) {
				throw std::system_error{ static_cast<SQLite3Error>(ec) };
			}
		}
		_database = nullptr;
	}
}

bool Database::is_open() const noexcept
{
	return _database;
}

void Database::open(const char* file, Open_flags flags, const char* vfs)
{
	if (vfs && !vfs[0]) {
		throw std::system_error{ Error::bad_vfs_name };
	}
	close();
	if (const auto ec = sqlite3_open_v2(file, &_database, flags, vfs)) {
		_database = nullptr;
		throw std::system_error{ static_cast<SQLite3Error>(ec) };
	}
}

void Database::register_function(const char* name, std::unique_ptr<function::Function> function)
{
	if (!is_open()) {
		throw std::system_error{ Error::database_is_closed };
	} else if (!function) {
		throw std::system_error{ Error::null_function };
	}
	const auto ec = sqlite3_create_function_v2(_database, name, function->_argc, function->_flags,
	                                           function.get(), &function::Function::xfunc, nullptr, nullptr,
	                                           [](void* ptr) { delete static_cast<function::Function*>(ptr); });
	if (ec) {
		throw std::system_error{ static_cast<SQLite3Error>(ec) };
	}
	function.release();
}

void Database::register_function(const char* name, void (*function)(sqlite3_context*, int, sqlite3_value**),
                                 function::text_encoding encoding, bool deterministic, bool direct_only)
{
	if (!is_open()) {
		throw std::system_error{ Error::database_is_closed };
	} else if (!function) {
		throw std::system_error{ Error::null_function };
	}
	const auto ec = sqlite3_create_function_v2(
	  _database, name, -1,
	  static_cast<int>(encoding) | SQLITE_DETERMINISTIC * deterministic | SQLITE_DIRECTONLY * direct_only,
	  nullptr, function, nullptr, nullptr, nullptr);
	if (ec) {
		throw std::system_error{ static_cast<SQLite3Error>(ec) };
	}
}

std::size_t Database::execute(const char* sql)
{
	if (!is_open()) {
		throw std::system_error{ Error::database_is_closed };
	}

	char* message      = nullptr;
	std::size_t result = 0;
	const int ec       = sqlite3_exec(
    _database, sql,
    [](void* result, int, char**, char**) {
      *static_cast<std::size_t*>(result) += 1;
      return SQLITE_OK;
    },
    &result, &message);
	const auto _ = finally([message] { sqlite3_free(message); });
	if (ec) {
		if (message) {
			throw std::system_error{ static_cast<SQLite3Error>(ec), message };
		}
		throw std::system_error{ static_cast<SQLite3Error>(ec) };
	}
	return result;
}

sqlite3_int64 Database::last_insert_rowid() const noexcept
{
	return is_open() ? sqlite3_last_insert_rowid(_database) : 0;
}

Statement Database::prepare_statement(const char* sql)
{
	if (!is_open()) {
		throw std::system_error{ Error::database_is_closed };
	}

	sqlite3_stmt* stmt = nullptr;
	const char* tail   = nullptr;
	if (const auto ec = sqlite3_prepare_v2(_database, sql, -1, &stmt, &tail)) {
		throw std::system_error{ static_cast<SQLite3Error>(ec), sqlite3_errmsg(_database) };
	}
	return { stmt, _database };
}

Transaction Database::begin_transaction()
{
	return Transaction{ *this };
}

sqlite3* Database::release_handle() noexcept
{
	const auto tmp = _database;
	_database      = nullptr;
	return tmp;
}

sqlite3* Database::handle() noexcept
{
	return _database;
}

const sqlite3* Database::handle() const noexcept
{
	return _database;
}

Database& Database::operator=(Database&& move) noexcept
{
	close(true);
	std::swap(_database, move._database);
	return *this;
}

} // namespace ysqlite3
