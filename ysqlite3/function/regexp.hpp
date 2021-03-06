#ifndef YSQLITE3_FUNCTION_REGEXP_HPP_
#define YSQLITE3_FUNCTION_REGEXP_HPP_

#include "function.hpp"

namespace ysqlite3 {
namespace function {

class Regexp : public Function
{
public:
	Regexp() noexcept;

protected:
	void run(sqlite3_context* context, int argc, sqlite3_value** argv) override;
};

} // namespace function
} // namespace ysqlite3

#endif
