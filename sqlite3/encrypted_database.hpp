#pragma once

#include <memory>
#include <string>

#include "config.hpp"
#include "database.hpp"
#include "encryption_context.hpp"


namespace ysqlite3
{

class encrypted_database : public database
{
public:
	SQLITE3_API encrypted_database(std::shared_ptr<encryption_context> _context, std::string _path, int _mode = O_READWRITE | O_CREATE);

	SQLITE3_API void rekey(const key_t & _new_key);
	SQLITE3_API void unlock(const key_t & _key);
	SQLITE3_API const std::shared_ptr<encryption_context> & context() noexcept;

protected:
	std::shared_ptr<encryption_context> _context;

	SQLITE3_API encrypted_database();
};

}