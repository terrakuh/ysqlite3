#pragma once

#include "database.hpp"
#include "encryption_context.hpp"


namespace sqlite3
{

class encrypted_database : public database
{
public:
	encrypted_database(encryption_context _context, const char * _filename, int _mode = SO_READWRITE | SO_CREATE);

protected:
	encryption_context _context;

	SQLITE3_API virtual bool open(const char * _filename, int _mode) override;
};

}