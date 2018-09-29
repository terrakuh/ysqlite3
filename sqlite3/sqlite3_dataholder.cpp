#include "sqlite3_dataholder.hpp"


namespace sqlite3
{

#include "sqlite3.h"

sqlite3_dataholder::sqlite3_dataholder()
{
	_handle = nullptr;
}

sqlite3_dataholder::~sqlite3_dataholder()
{
	if (_handle) {
		sqlite3_close_v2(handle<sqlite3>());
	}
}

}