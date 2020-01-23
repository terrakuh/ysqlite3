#ifndef YSQLITE3_EXCEPTION_PARAMETER_EXCEPTION_HPP_
#define YSQLITE3_EXCEPTION_PARAMETER_EXCEPTION_HPP_

#include "base_exception.hpp"

namespace ysqlite3 {
namespace exception {

class parameter_exception : public base_exception
{
public:
	using base_exception::base_exception;
};

} // namespace exception
} // namespace ysqlite3

#endif