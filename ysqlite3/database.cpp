#include "database.hpp"

#include "error.hpp"
#include "finally.hpp"

#include <utility>

using namespace ysqlite3;

database::database(const char* file)
{
	open(file);
}

database::database(database&& move) noexcept
{
	std::swap(_database, move._database);
}

database::~database()
{
	close(true);
}

std::uint8_t database::set_reserved_size(std::uint8_t size, bool vacuum)
{
	if (!is_open()) {
		throw std::system_error{ errc::database_is_closed };
	}

	int n   = size;
	auto ec = sqlite3_file_control(_database, nullptr, SQLITE_FCNTL_RESERVE_BYTES, &n);
	if (!ec && vacuum && n != static_cast<int>(size)) {
		ec = sqlite3_exec(_database, "VACUUM", nullptr, nullptr, nullptr);
		// reset
		if (ec) {
			sqlite3_file_control(_database, nullptr, SQLITE_FCNTL_RESERVE_BYTES, &n);
		}
	}
	if (ec) {
		throw std::system_error{ static_cast<sqlite3_errc>(ec) };
	}
	return static_cast<std::uint8_t>(size);
}

void database::set_journal_mode(journal_mode mode)
{
	switch (mode) {
	case journal_mode::delete_: execute("PRAGMA journal_mode=DELETE;"); break;
	case journal_mode::truncate: execute("PRAGMA journal_mode=TRUNCATE;"); break;
	case journal_mode::persist: execute("PRAGMA journal_mode=PERSIST;"); break;
	case journal_mode::memory: execute("PRAGMA journal_mode=MEMORY;"); break;
	case journal_mode::wal: execute("PRAGMA journal_mode=WAL;"); break;
	case journal_mode::off: execute("PRAGMA journal_mode=OFF;"); break;
	}
}

void database::enable_foreign_keys(bool enable)
{
	execute(enable ? "PRAGMA foreign_keys=ON" : "PRAGMA foreign_keys=OFF");
}

void database::close(bool force)
{
	if (_database) {
		if (force) {
			sqlite3_close_v2(_database);
		} else {
			if (const auto ec = sqlite3_close(_database)) {
				throw std::system_error{ static_cast<sqlite3_errc>(ec) };
			}
		}

		_database = nullptr;
	}
}

bool database::is_open() const noexcept
{
	return _database;
}

void database::open(const char* file, open_flag_type flags, const char* vfs)
{
	if (vfs && !vfs[0]) {
		throw std::system_error{ errc::bad_vfs_name };
	}

	close();

	if (const auto ec = sqlite3_open_v2(file, &_database, flags, vfs)) {
		_database = nullptr;
		throw std::system_error{ static_cast<sqlite3_errc>(ec) };
	}
}

std::size_t database::execute(const char* sql)
{
	if (!is_open()) {
		throw std::system_error{ errc::database_is_closed };
	}

	char* message      = nullptr;
	std::size_t result = 0;
	const auto ec      = sqlite3_exec(
        _database, sql,
        [](void* result, int, char**, char**) {
            *static_cast<std::size_t*>(result) += 1;
            return SQLITE_OK;
        },
        &result, &message);

	const auto _ = finally([message] { sqlite3_free(message); });

	if (ec) {
		if (message) {
			throw std::system_error{ static_cast<sqlite3_errc>(ec), message };
		}

		throw std::system_error{ static_cast<sqlite3_errc>(ec) };
	}

	return result;
}

sqlite3_int64 database::last_insert_rowid() const noexcept
{
	return is_open() ? sqlite3_last_insert_rowid(_database) : 0;
}

statement database::prepare_statement(const char* sql)
{
	if (!is_open()) {
		throw std::system_error{ errc::database_is_closed };
	}

	sqlite3_stmt* stmt = nullptr;
	const char* tail   = nullptr;

	if (const auto ec = sqlite3_prepare_v2(_database, sql, -1, &stmt, &tail)) {
		throw std::system_error{ static_cast<sqlite3_errc>(ec) };
	}

	return { stmt, _database };
}

sqlite3* database::release_handle() noexcept
{
	const auto tmp = _database;
	_database      = nullptr;
	return tmp;
}

sqlite3* database::handle() noexcept
{
	return _database;
}

const sqlite3* database::handle() const noexcept
{
	return _database;
}

database& database::operator=(database&& move) noexcept
{
	close(true);
	std::swap(_database, move._database);
	return *this;
}
