#include "database.hpp"

namespace sqlite3
{

#include "sqlite3.h"

struct database::impl
{
	sqlite3 * connection;

	impl()
	{
		connection = nullptr;
	}
	~impl()
	{
		if (connection) {
			sqlite3_close_v2(connection);
		}
	}
};

database::database(const char * _filename, int _mode) : _impl(new impl())
{
	if (!open(_filename, _mode)) {
		throw database_error(sqlite3_errmsg(_impl->connection));
	}
}

void database::execute(const char * _sql)
{
	char * _error_msg = nullptr;

	if (sqlite3_exec(_impl->connection, _sql, nullptr, nullptr, &_error_msg) != SQLITE_OK) {
		std::logic_error _exception(_error_msg);

		// Free message
		sqlite3_free(_error_msg);

		throw _exception;
	}
}

bool database::open(const char * _filename, int _mode)
{
	return sqlite3_open_v2(_filename, &_impl->connection, _mode, nullptr) == SQLITE_OK;
}

}