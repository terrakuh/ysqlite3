#include "encrypted_database.hpp"
#include "sqlite3.h"
#include "address_transporter.hpp"


namespace ysqlite3
{

encrypted_database::encrypted_database(std::shared_ptr<encryption_context> _context, std::string _path, int _mode) : _context(std::move(_context))
{
	address_transporter::encode_address(_path, &this->_context);

	if (sqlite3_open_v2(_path.c_str(), &_connection->get<sqlite3>(), _mode, SQLITE3_CUSTOM_VFS_NAME) != SQLITE_OK) {
		throw database_error(sqlite3_errmsg(_connection->get<sqlite3>()));
	}

	// Set some options if newly created
	if (this->_context->newly_created()) {
		sqlite3_test_control(SQLITE_TESTCTRL_RESERVE, _connection->get<sqlite3>(), SQLITE3_MAX_USER_DATA_SIZE);
	}
}

void encrypted_database::rekey(const encryption_context::key_t & _new_key)
{
	_context->set_key(_new_key, true);

	execute("VACUUM;");

	_context->set_key(_new_key, false);
}

void encrypted_database::unlock(const encryption_context::key_t & _key)
{
	_context->set_key(_key, true);
	_context->set_key(_key, false);
}

const std::shared_ptr<encryption_context> & encrypted_database::context() noexcept
{
	return _context;
}

encrypted_database::encrypted_database()
{
}

}
