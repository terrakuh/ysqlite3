#include "database.hpp"

namespace sqlite3
{

#include "sqlite3.h"

database::database(const char * _filename, int _mode) : _connection(new handle([](void * _connection) { sqlite3_close_v2(reinterpret_cast<sqlite3*>(_connection)); }))
{
	if (!open(_filename, _mode)) {
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

bool database::open(const char * _filename, int _mode)
{
	return sqlite3_open_v2(_filename, &_connection->get<sqlite3>(), _mode, nullptr) == SQLITE_OK;
}

}