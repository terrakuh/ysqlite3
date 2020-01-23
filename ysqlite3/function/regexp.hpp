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
	regexp(bool cache = true) noexcept
	    : function(2, true, false, text_enconding::utf8),
	      _cache(cache ? new std::map<std::string, std::regex>() : nullptr)
	{}

protected:
	virtual void run(sqlite3_context* context, int argc, sqlite3_value** argv) override
	{
		std::regex p;
		std::regex* pattern = nullptr;

		// cached
		if (_cache) {
			std::string str(reinterpret_cast<const char*>(sqlite3_value_text(argv[0])));
			auto r = _cache->find(str);

			if (r != _cache->end()) {
				pattern = &r->second;
			} else {
				p       = std::regex(str, std::regex_constants::ECMAScript | std::regex_constants::optimize);
				pattern = &_cache->emplace(std::move(str), std::move(p)).first->second;
			}
		} else {
			p       = std::regex(reinterpret_cast<const char*>(sqlite3_value_text(argv[0])));
			pattern = &p;
		}

		std::cmatch match;

		if (std::regex_match(reinterpret_cast<const char*>(sqlite3_value_text(argv[1])), match, *pattern)) {
			sqlite3_result_int(context, 1);
		} else {
			sqlite3_result_int(context, 0);
		}
	}

private:
	std::unique_ptr<std::map<std::string, std::regex>> _cache;
};

} // namespace function
} // namespace ysqlite3

#endif