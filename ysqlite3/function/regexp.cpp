#include "regexp.hpp"

#include "../error.hpp"

#include <regex>

using namespace ysqlite3::function;

Regexp::Regexp() noexcept : Function{ 2, true, false, text_encoding::utf8 }
{}

void Regexp::run(sqlite3_context* context, int argc, sqlite3_value** argv)
{
	if (argc != 2) {
		throw std::system_error{ SQLite3Error::generic, "requires 2 arguments" };
	}

	// get or create pattern
	auto pattern = static_cast<std::regex*>(sqlite3_get_auxdata(context, 0));
	if (!pattern) {
		const auto str = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));
		if (!str) {
			throw std::system_error{ SQLite3Error::generic, "bad regex" };
		}
		// compile
		try {
			pattern =
			    new std::regex{ str, std::regex_constants::ECMAScript | std::regex_constants::optimize };
		} catch (const std::regex_error& e) {
			throw std::system_error{ SQLite3Error::generic, e.what() };
		}
		// cache for later use
		sqlite3_set_auxdata(context, 0, pattern, [](void* ptr) { delete static_cast<std::regex*>(ptr); });
	}
	// match
	std::cmatch match;
	const auto str = reinterpret_cast<const char*>(sqlite3_value_text(argv[1]));
	sqlite3_result_int(context, static_cast<int>(str && std::regex_match(str, match, *pattern)));
}
