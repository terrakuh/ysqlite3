#include "encrypted_database.hpp"
#include "encrypted_vfs.hpp"
#include "sqlite3.h"

#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <boost/lexical_cast.hpp>


namespace ysqlite3
{

encrypted_database::encrypted_database(std::shared_ptr<encryption_context> _context, const char * _path, int _mode)
{
	std::stringstream _stream;

	_stream << _path << '-' << std::hex << reinterpret_cast<intptr_t>(&_context) << '-';

	if (sqlite3_open_v2(_stream.str().c_str(), &_connection->get<sqlite3>(), _mode, SQLITE3_ENCRYPTED_VFS_NAME) != SQLITE_OK) {
		throw database_error(sqlite3_errmsg(_connection->get<sqlite3>()));
	}
	puts("update");
	sqlite3_test_control(SQLITE_TESTCTRL_RESERVE, _connection->get<sqlite3>(), SQLITE3_MAX_USER_DATA_SIZE);

	execute("PRAGMA journal_mode=MEMORY;");
}

encrypted_database::encrypted_database()
{
}

}
