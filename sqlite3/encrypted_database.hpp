#pragma once

#include <memory>

#include "config.hpp"
#include "database.hpp"
#include "encryption_context.hpp"


namespace ysqlite3
{

class encrypted_database : public database
{
public:
	SQLITE3_API encrypted_database(encryption_context * _context, const char * _path, int _mode = SO_READWRITE | SO_CREATE);
protected:
	encryption_context * _context;

	encrypted_database();
};

}