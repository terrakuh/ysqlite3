#pragma once

#include "cast.hpp"
#include "error.hpp"
#include "span.hpp"
#include "sqlite3.h"
#include "util/templated_of.hpp"

#include <cstdint>
#include <limits>
#include <locale>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

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
		if constexpr (std::is_integral_v<Type>) {
			return numeric_cast<Type>(integer(index));
		} else if constexpr (std::is_floating_point_v<Type>) {
			return static_cast<Type>(real(index));
		} else if constexpr (std::is_same_v<Type, std::string> || std::is_same_v<Type, std::string_view>) {
			const auto ptr = text(index);
			return Type{ ptr, numeric_cast<std::size_t>(size_of(index)) };
		} else if constexpr (util::TemplatedOf<Type, std::optional>::value) {
			if (is_null(index)) {
				return std::nullopt;
			}
			return get<typename Type::value_type>(index);
		} else {
			_assert_get();
		}
	}
	template<typename... Types>
	[[nodiscard]] std::tuple<Types...> get()
	{
		return _get<Types...>(std::make_integer_sequence<int, static_cast<int>(sizeof...(Types))>{});
	}
	[[nodiscard]] bool is_valid() const noexcept;
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
	template<typename... Types, int... Indices>
	std::tuple<Types...> _get(std::integer_sequence<int, Indices...> /* indices */)
	{
		return std::make_tuple(get<Types>(Indices)...);
	}
	template<bool Flag = false>
	void _assert_get()
	{
		static_assert(Flag, "Unsupported type.");
	}
};

} // namespace ysqlite3
