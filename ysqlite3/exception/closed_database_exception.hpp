#pragma once

#include "base_exception.hpp"

namespace ysqlite3 {
namespace exception {

/**
 Thrown when an operation was requested on a closed database.
*/
class closed_database_exception : public base_exception
{
public:
    using base_exception::base_exception;
};

} // namespace exception
} // namespace ysqlite3