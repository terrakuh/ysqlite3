#include "regexp.hpp"

using namespace ysqlite3::function;

regexp::regexp(bool cache) noexcept
    : function(2, true, false, text_encoding::utf8),
      _cache(cache ? new std::map<std::string, std::regex>() : nullptr)
{}

void regexp::run(sqlite3_context* context, int argc, sqlite3_value** argv)
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
