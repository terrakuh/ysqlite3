#ifndef YSQLITE3_FUNCTION_FUNCTION_HPP_
#define YSQLITE3_FUNCTION_FUNCTION_HPP_

#include "../error.hpp"
#include "../sqlite3.h"

namespace ysqlite3 {

class database;

namespace function {

/**
 * A function interface for SQLite.
 */
class function
{
public:
	enum class text_encoding
	{
		utf8    = SQLITE_UTF8,
		utf16be = SQLITE_UTF16BE,
		utf16le = SQLITE_UTF16LE
	};

	/**
	 * Constructor.
	 *
	 * @param argc the maximum allowed parameter count; if this value is negative the is no limit
	 * @param deterministic whether this function produces the same output with the same input
	 * @param encoding the preferred text encoding
	 * @see more information can be found on the [SQLite
	 * page](https://www.sqlite.org/c3ref/create_function.html)
	 */
	function(int argc, bool deterministic, bool direct_only, text_encoding encoding) noexcept
	{
		_argc  = argc < 0 ? -1 : argc;
		_flags = static_cast<int>(encoding) | (deterministic ? SQLITE_DETERMINISTIC : 0) |
		         (direct_only ? SQLITE_DIRECTONLY : 0);
	}
	virtual ~function() = default;
	/**
	 * The function interface for SQLite.
	 *
	 * @param[in] context the context
	 * @param argc the argument count
	 * @param[in] argv the argument values
	 */
	static void xfunc(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept
	{
		try {
			static_cast<function*>(sqlite3_user_data(context))->run(context, argc, argv);
		} catch (const std::exception& e) {
			sqlite3_result_error(context, e.what(),
			                     dynamic_cast<const std::system_error*>(&e)
			                         ? static_cast<const std::system_error&>(e).code().value()
			                         : -1);
		} catch (...) {
			sqlite3_result_error(context, "function threw an exception", -1);
		}
	}

protected:
	/**
	 * Runs the function.
	 *
	 * @param[in] context the function context
	 * @param argc the argument count
	 * @param[in] argv the argument values
	 */
	virtual void run(sqlite3_context* context, int argc, sqlite3_value** argv) = 0;

private:
	friend database;

	int _argc;
	int _flags;
};

} // namespace function
} // namespace ysqlite3

#endif
