#pragma once

#include "error.hpp"
#include "cast.hpp"
#include "span.hpp"
#include "sqlite3.h"

#include <cstdint>
#include <locale>
#include <type_traits>
#include <tuple>

namespace ysqlite3 {

struct Index
{
	int value        = 0;
	const char* name = nullptr;
	std::locale locale;

	Index(int value) noexcept : value{ value }
	{
	}
	Index(const char* name, std::locale locale = {}) : name{ name }, locale{ std::move(locale) }
	{
		if (!name) {
			throw std::system_error{ Error::bad_index_name };
		}
	}
};

/**
 * Retrieves the result from a successful statement::step().  This object is only valid, until another
 * non-const function of @ref statement is called or until its destruction.
 *
 * @see more information about conversion can be found [here](https://www.sqlite.org/c3ref/column_blob.html)
 */
class Results
{
public:
	enum class type
	{
		integer,
		real,
		text,
		blob,
		null,
	};

	Results() = default;
	/**
	 * Constructor. Either both parameters are `nullptr` or none.
	 *
	 * @param[in] statement the statement
	 * @param[in] database the database
	 */
	Results(sqlite3_stmt* statement, sqlite3* database);
	/**
	 * Checks if the value at the index is null.
	 *
	 * @pre `*this == true`
	 *
	 * @exception see _to_column_index()
	 * @param index the column
	 * @return `true` if null, otherwise `false`
	 */
	bool is_null(Index index);
	/**
	 * Returns the value at the index as an integer.
	 *
	 * @pre `*this == true`
	 *
	 * @exception see _to_column_index()
	 * @param index the column
	 * @return the integer value
	 */
	sqlite3_int64 integer(Index index);
	/**
	 * Returns the value at the index as a double.
	 *
	 * @pre `*this == true`
	 *
	 * @exception see _to_column_index()
	 * @param index the column
	 * @return the double value
	 */
	double real(Index index);
	/**
	 * Returns the value at the index as a blob.
	 *
	 * @pre `*this == true`
	 *
	 * @exception see _to_column_index()
	 * @param index the column
	 * @return the blob of data
	 */
	Span<const std::uint8_t*> blob(Index index);
	/**
	 * Returns the value at the index as text.
	 *
	 * @pre `*this == true`
	 *
	 * @exception see _to_column_index()
	 * @param index the column
	 * @return the text
	 */
	const char* text(Index index);
	/**
	 * Returns the size of the value at the index.
	 *
	 * @pre `*this == true`
	 *
	 * @exception see _to_column_index()
	 * @param index the column
	 * @return the size in bytes
	 */
	int size_of(Index index);
	/**
	 * Returns the type of the value at the index.
	 *
	 * @pre `*this == true`
	 *
	 * @exception see _to_column_index()
	 * @param index the column
	 * @return the type
	 */
	type type_of(Index index);
	template<typename Type>
	[[nodiscard]] Type get(Index index)
	{
		if constexpr (std::is_integral_v<Type> && !std::is_same_v<Type, bool>) {
			return numeric_cast<Type>(integer(index));
		} else if constexpr (std::is_floating_point_v<Type>) {
			return static_cast<Type>(real(index));
		}
	}
	template<typename... Types>
	[[nodiscard]] std::tuple<Types...> get()
	{
		int index = 0;
		return std::make_tuple(get<Types>(index++)...);
	}
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
	int _to_column_index(Index index);
};

} // namespace ysqlite3
