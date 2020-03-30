#ifndef YSQLITE3_RESULTS_HPP_
#define YSQLITE3_RESULTS_HPP_

#include "sqlite3.h"

#include <gsl/gsl>
#include <locale>

namespace ysqlite3 {

struct index
{
	int value        = 0;
	const char* name = nullptr;
	std::locale locale;

	index(int value) noexcept : value(value)
	{}
	index(gsl::czstring<> name, const std::locale& locale = {}) : name(name), locale(std::move(locale))
	{
		Expects(name);
	}
};

/**
 *
 * @see more information about conversion can be found [here](https://www.sqlite.org/c3ref/column_blob.html)
 */
class results
{
public:
	enum class type
	{
		integer,
		real,
		text,
		blob,
		null
	};

	results() = default;
	results(sqlite3_stmt* statement, sqlite3* database) noexcept;

	bool is_null(index index);
	int integer(index index);
	sqlite3_int64 integer64(index index);
	double real(index index);
	gsl::span<const gsl::byte> blob(index index);
	gsl::czstring<> text(index index);
	int size_of(index index);
	type type_of(index index);
	operator bool() const noexcept;

private:
	sqlite3_stmt* _statement = nullptr;
	sqlite3* _database       = nullptr;

	int _to_column_index(index index);
};

} // namespace ysqlite3

#endif