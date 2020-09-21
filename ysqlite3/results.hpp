#ifndef YSQLITE3_RESULTS_HPP_
#define YSQLITE3_RESULTS_HPP_

#include "error.hpp"
#include "span.hpp"
#include "sqlite3.h"

#include <cstdint>
#include <locale>

namespace ysqlite3 {

struct index
{
	int value        = 0;
	const char* name = nullptr;
	std::locale locale;

	index(int value) noexcept : value{ value }
	{}
	index(const char* name, std::locale locale = {}) : name{ name }, locale{ std::move(locale) }
	{
		if (!name) {
			throw std::system_error{ errc::bad_index_name };
		}
	}
};

/**
 * Retrieves the result from a successful statement::step().  This object is only valid, until another
 * non-const function of @ref statement is called or until its destruction.
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
	 * Constructor. Either both parameters are `nullptr` or none.
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
	 * @exception see _to_column_index()
	 * @param index the column
	 * @return `true` if null, otherwise `false`
	 */
	bool is_null(index index);
	/**
	 * Returns the value at the index as an integer.
	 *
	 * @pre `*this == true`
	 *
	 * @exception see _to_column_index()
	 * @param index the column
	 * @return the integer value
	 */
	sqlite3_int64 integer(index index);
	/**
	 * Returns the value at the index as a double.
	 *
	 * @pre `*this == true`
	 *
	 * @exception see _to_column_index()
	 * @param index the column
	 * @return the double value
	 */
	double real(index index);
	/**
	 * Returns the value at the index as a blob.
	 *
	 * @pre `*this == true`
	 *
	 * @exception see _to_column_index()
	 * @param index the column
	 * @return the blob of data
	 */
	span<const std::uint8_t*> blob(index index);
	/**
	 * Returns the value at the index as text.
	 *
	 * @pre `*this == true`
	 *
	 * @exception see _to_column_index()
	 * @param index the column
	 * @return the text
	 */
	const char* text(index index);
	/**
	 * Returns the size of the value at the index.
	 *
	 * @pre `*this == true`
	 *
	 * @exception see _to_column_index()
	 * @param index the column
	 * @return the size in bytes
	 */
	int size_of(index index);
	/**
	 * Returns the type of the value at the index.
	 *
	 * @pre `*this == true`
	 *
	 * @exception see _to_column_index()
	 * @param index the column
	 * @return the type
	 */
	type type_of(index index);
	/**
	 * Checks if this has a statement.
	 *
	 * @return `true` if a statement is set, otherwise `false`
	 */
	operator bool() const noexcept;

private:
	sqlite3_stmt* _statement = nullptr;
	sqlite3* _database       = nullptr;

	/**
	 * Returns the integer index of the column.
	 *
	 * @exception std::system_error if the column name is unknown or the index is out of range
	 * @param index the column
	 * @return the integer index
	 */
	int _to_column_index(index index);
};

} // namespace ysqlite3

#endif
