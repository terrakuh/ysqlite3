#pragma once

#include "../sqlite3.h"
#include "base_exception.hpp"

namespace ysqlite3 {
namespace exception {

class database_exception : public base_exception
{
public:
    database_exception(int error_code, const std::string& msg, const char* file, const char* function, int line)
        : base_exception(msg + " (error_code=" + std::to_string(error_code) + "; " + sqlite3_errstr(error_code), file,
                         function, line)
    {}
};

} // namespace exception
} // namespace ysqlite3