#include "database.hpp"
#include "sqlite3.h"


namespace ysqlite3
{

database::database(const char * _filename, int _mode) : database()
{
	if (sqlite3_open_v2(_filename, &_connection->get<sqlite3>(), _mode, nullptr) != SQLITE_OK) {
		throw database_error(sqlite3_errmsg(_connection->get<sqlite3>()));
	}
}

void database::execute(const char * _sql)
{
	char * _error_msg = nullptr;

	if (sqlite3_exec(_connection->get<sqlite3>(), _sql, nullptr, nullptr, &_error_msg) != SQLITE_OK) {
		programming_error _exception(_error_msg);

		// Free message
		sqlite3_free(_error_msg);

		throw _exception;
	}
}

void database::enable_foreign_keys(bool _enable)
{
	execute(_enable ? "PRAGMA foreign_keys=ON" : "PRAGMA foreign_keys=OFF");
}

void database::begin_transaction()
{
	execute("BEGIN TRANSACTION;");
}

void database::commit()
{
	execute("COMMIT;");
}

void database::rollback()
{
	execute("ROLLBACK;");
}

void database::set_journal_mode(JOURNAL_MODE _mode)
{
	switch (_mode) {
	case JOURNAL_MODE::DELETE:
		execute("PRAGMA journal_mode=DELETE;");

		break;
	case JOURNAL_MODE::TRUNCATE:
		execute("PRAGMA journal_mode=TRUNCATE;");

		break;
	case JOURNAL_MODE::PERSIST:
		execute("PRAGMA journal_mode=PERSIST;");

		break;
	case JOURNAL_MODE::MEMORY:
		execute("PRAGMA journal_mode=MEMORY;");

		break;
	case JOURNAL_MODE::WAL:
		execute("PRAGMA journal_mode=WAL;");

		break;
	case JOURNAL_MODE::OFF:
		execute("PRAGMA journal_mode=OFF;");

		break;
	default:
		break;
	}
}

bool database::is_readonly() const
{
	auto _result = sqlite3_db_readonly(_connection->get<sqlite3>(), "main");

	if (_result < 0) {
		throw database_error("invalid database");
	}

	return _result;
}

long long database::last_insert_rowid() const noexcept
{
	return sqlite3_last_insert_rowid(_connection->get<sqlite3>());
}

statement database::create_statement(const char * _sql)
{
	statement _stmt(_connection);

	if (_sql) {
		_stmt.set_sql(_sql);
	}

	return _stmt;
}

database::database() : _connection(new handle([](void * _connection) { sqlite3_close_v2(reinterpret_cast<sqlite3*>(_connection)); }))
{
}

}