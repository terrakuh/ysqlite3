#include "database.hpp"

namespace sqlite3
{

#include "sqlite3.h"

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

long long database::last_insert_rowid() const noexcept
{
	return sqlite3_last_insert_rowid(_connection->get<sqlite3>());
}

database::database() : _connection(new handle([](void * _connection) { sqlite3_close_v2(reinterpret_cast<sqlite3*>(_connection)); }))
{
}

}