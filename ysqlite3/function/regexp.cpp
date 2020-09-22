#include "regexp.hpp"

#include "../error.hpp"

#include <regex>

using namespace ysqlite3::function;

regexp::regexp() noexcept : function{ 2, true, false, text_encoding::utf8 }
{}

void regexp::run(sqlite3_context* context, int argc, sqlite3_value** argv)
{
	if (argc != 2) {
		throw std::system_error{ sqlite3_errc::generic, "requires 2 arguments" };
	}

	auto pattern = static_cast<std::regex*>(sqlite3_get_auxdata(context, 0));

	// not compiled
	if (!pattern) {
		const auto str = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));

		if (!str) {
			throw std::system_error{ sqlite3_errc::generic, "bad regex" };
		}

		try {
			pattern =
			    new std::regex{ str, std::regex_constants::ECMAScript | std::regex_constants::optimize };
		} catch (const std::regex_error& e) {
			throw std::system_error{ sqlite3_errc::generic, "bad regex" };
		}

		sqlite3_set_auxdata(context, 0, pattern, [](void* ptr) { delete static_cast<std::regex*>(ptr); });
	}

	std::cmatch match;
	const auto str = reinterpret_cast<const char*>(sqlite3_value_text(argv[1]));
	sqlite3_result_int(context, static_cast<int>(str && std::regex_match(str, match, *pattern)));
}
