#pragma once

#include "base_exception.hpp"

namespace ysqlite3 {
namespace exception {

class closed_statement_exception : public base_exception
{
public:
    using base_exception::base_exception;
};

} // namespace exception
} // namespace ysqlite3