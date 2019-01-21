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
	SQLITE3_API encrypted_database(std::shared_ptr<encryption_context> _context, const char * _path, int _mode = O_READWRITE | O_CREATE);

protected:
	std::shared_ptr<encryption_context> _context;

	encrypted_database();
};

}