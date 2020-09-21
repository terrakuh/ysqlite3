#ifndef YSQLITE3_FUNCTION_REGEXP_HPP_
#define YSQLITE3_FUNCTION_REGEXP_HPP_

#include "function.hpp"

#include <map>
#include <memory>
#include <regex>
#include <string>

namespace ysqlite3 {
namespace function {

class regexp : public function
{
public:
	regexp(bool cache = true) noexcept;

protected:
	void run(sqlite3_context* context, int argc, sqlite3_value** argv) override;

private:
	std::unique_ptr<std::map<std::string, std::regex>> _cache;
};

} // namespace function
} // namespace ysqlite3

#endif