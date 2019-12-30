#pragma once

#include "sqlite3_exception.hpp"

namespace ysqlite3 {
namespace exception {

/**
 Thrown if an operation on the database failed. A database error has an additional SQLite3 specific error code.
*/
class database_exception : public sqlite3_exception
{
public:
    using sqlite3_exception::sqlite3_exception;
};

} // namespace exception
} // namespace ysqlite3