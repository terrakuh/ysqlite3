#include "encrypted_database.hpp"
#include "encrypted_vfs.hpp"

#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <boost\lexical_cast.hpp>


namespace sqlite3
{

#include "sqlite3.h"


encrypted_database::encrypted_database(encryption_context * _context, const char * _filename, int _mode)
{
	std::stringstream _s;

	_s << std::hex << reinterpret_cast<intptr_t>(_context) << '-' << _filename;

	if (sqlite3_open_v2(_s.str().c_str(), &_connection->get<sqlite3>(), _mode, SQLITE3_ENCRYPTED_VFS_NAME) != SQLITE_OK) {
		throw database_error(sqlite3_errmsg(_connection->get<sqlite3>()));
	}
}

encrypted_database::encrypted_database()
{
}

}