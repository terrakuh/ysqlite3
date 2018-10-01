#include "encrypted_database.hpp"
#include "encrypted_vfs.hpp"

#include <string>
#include <boost\lexical_cast.hpp>


namespace sqlite3
{

#include "sqlite3.h"


encrypted_database::encrypted_database(encryption_context _context, const char * _filename, int _mode) : database(_filename, _mode)
{
}

bool encrypted_database::open(const char * _filename, int _mode)
{
	return sqlite3_open_v2(_filename, &_connection->get<sqlite3>(), _mode, SQLITE3_ENCRYPTED_VFS_NAME) == SQLITE_OK;
}

}