#pragma once

#include "../sqlite3.h"
#include "base_exception.hpp"

namespace ysqlite3 {
namespace exception {

/**
 Thrown if an operation on the database failed. A database error has an additional SQLite3 specific error code.
*/
class database_exception : public base_exception
{
public:
    database_exception(int error_code, const std::string& msg, const char* file, const char* function, int line)
        : base_exception(msg + " (error_code=" + std::to_string(error_code) + "; " + sqlite3_errstr(error_code) + ")", file,
                         function, line)
    {
        _error_code = error_code;
    }
    /**
     Returns the error code of the exception.

     @returns a SQLite3 specific error code
    */
    int error_code() const noexcept
    {
        return _error_code;
    }

private:
    int _error_code;
};

} // namespace exception
} // namespace ysqlite3