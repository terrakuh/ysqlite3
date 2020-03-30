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
 * Retrieves the result from a successful statement::step().
 *
 * @note This object is only valid, until another non-const function of @ref statement is called or until its
 * destruction.
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
	/**
	 * Constructor.
	 *
	 * @pre either both parameters are `nullptr` or none
	 *
	 * @param[in] statement the statement
	 * @param[in] database the database
	 */
	results(sqlite3_stmt* statement, sqlite3* database) noexcept;

	/**
	 * Checks if the value at the index is null.
	 *
	 * @pre `*this == true`
	 *
	 * @param index the column
	 * @returns `true` if null, otherwise `false`
	 * @throw see _to_column_index()
	 */
	bool is_null(index index);
	/**
	 * Returns the value at the index as an integer.
	 *
	 * @pre `*this == true`
	 *
	 * @param index the column
	 * @returns the integer value
	 * @throw see _to_column_index()
	 */
	sqlite3_int64 integer(index index);
	/**
	 * Returns the value at the index as a double.
	 *
	 * @pre `*this == true`
	 *
	 * @param index the column
	 * @returns the double value
	 * @throw see _to_column_index()
	 */
	double real(index index);
	/**
	 * Returns the value at the index as a blob.
	 *
	 * @pre `*this == true`
	 *
	 * @param index the column
	 * @returns the blob of data
	 * @throw see _to_column_index()
	 */
	gsl::span<const gsl::byte> blob(index index);
	/**
	 * Returns the value at the index as text.
	 *
	 * @pre `*this == true`
	 *
	 * @param index the column
	 * @returns the text
	 * @throw see _to_column_index()
	 */
	gsl::czstring<> text(index index);
	/**
	 * Returns the size of the value at the index.
	 *
	 * @pre `*this == true`
	 *
	 * @param index the column
	 * @returns the size in bytes
	 * @throw see _to_column_index()
	 */
	int size_of(index index);
	/**
	 * Returns the type of the value at the index.
	 *
	 * @pre `*this == true`
	 *
	 * @param index the column
	 * @returns the type
	 * @throw see _to_column_index()
	 */
	type type_of(index index);
	/**
	 * Checks if this has a statement.
	 *
	 * @returns `true` if a statement is set, otherwise `false`
	 */
	operator bool() const noexcept;

private:
	sqlite3_stmt* _statement = nullptr;
	sqlite3* _database       = nullptr;

	/**
	 * Returns the integer index of the column.
	 *
	 * @param index the column
	 * @returns the integer index
	 * @throw exception::parameter_exception if the column name is unknown or the index is out of range
	 */
	int _to_column_index(index index);
};

} // namespace ysqlite3

#endif