#include "sqlite3_connection.hpp"


namespace sqlite3
{

#include "sqlite3.h"

sqlite3_connection::sqlite3_connection()
{
	_handle = nullptr;
}

sqlite3_connection::~sqlite3_connection()
{
	if (_handle) {
		sqlite3_close_v2(handle<sqlite3>());
	}
}

}