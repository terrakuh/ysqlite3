#include "database.hpp"

#include <utility>

using namespace ysqlite3;
using transaction = database::transaction;

transaction::transaction(transaction&& move) noexcept
{
	_db      = move._db;
	move._db = nullptr;
}

transaction::~transaction()
{
	if (_db) {
		rollback();
	}
}

void transaction::rollback()
{
	Expects(open());

	_db->execute("ROLLBACK;");

	_db = nullptr;
}

bool transaction::open() const noexcept
{
	return _db;
}

transaction::operator bool() const noexcept
{
	return open();
}

transaction::transaction(gsl::not_null<database*> db) noexcept
{
	_db = db;
}

void transaction::commit()
{
	Expects(open());

	_db->execute("COMMIT;");

	_db = nullptr;
}

database::database(gsl::not_null<gsl::czstring<>> file)
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
			auto error = sqlite3_close(_database);

			if (error != SQLITE_OK) {
				YSQLITE_THROW(exception::database_exception, error, "cloud not close");
			}
		}

		_database = nullptr;
	}
}

bool database::closed() const noexcept
{
	return _database == nullptr;
}

void database::open(gsl::not_null<gsl::czstring<>> file, open_flag_type flags, gsl::czstring<> vfs)
{
	Expects(vfs == nullptr || vfs[0] != 0);

	close();

	auto error = sqlite3_open_v2(file, &_database, flags, vfs);

	if (error != SQLITE_OK) {
		_database = nullptr;

		YSQLITE_THROW(exception::database_exception, error, "failed to open");
	}

	Ensures(!closed());
}

std::size_t database::execute(gsl::not_null<gsl::czstring<>> sql)
{
	Expects(!closed());

	char* message      = nullptr;
	std::size_t result = 0;
	auto error         = sqlite3_exec(
        _database, sql,
        [](void* result, int, char**, char**) {
            *static_cast<std::size_t*>(result) += 1;

            return SQLITE_OK;
        },
        &result, &message);

	if (message) {
		auto _ = gsl::finally([message] { sqlite3_free(message); });

		YSQLITE_THROW(exception::sql_exception, message);
	} else if (error != SQLITE_OK) {
		YSQLITE_THROW(exception::database_exception, error, "failed to execute sql");
	}

	return result;
}

sqlite3_int64 database::last_insert_rowid() const
{
	Expects(!closed());

	return sqlite3_last_insert_rowid(_database);
}

statement database::prepare_statement(gsl::not_null<gsl::czstring<>> sql)
{
	Expects(!closed());

	sqlite3_stmt* stmt = nullptr;
	const char* tail   = nullptr;
	auto error         = sqlite3_prepare_v2(_database, sql, -1, &stmt, &tail);

	if (error != SQLITE_OK) {
		YSQLITE_THROW(exception::database_exception, error, "could not prepare statement");
	}

	return stmt;
}

database::transaction database::begin_transaction()
{
	execute("BEGIN TRANSACTION;");

	return { this };
}

gsl::owner<sqlite3*> database::release() noexcept
{
	auto p = _database;

	_database = nullptr;

	return p;
}

sqlite3* database::handle() noexcept
{
	return _database;
}

const sqlite3* database::handle() const noexcept
{
	return _database;
}