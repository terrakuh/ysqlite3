#pragma once

#include "cast.hpp"
#include "results.hpp"
#include "sqlite3.h"

#include <cstddef>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace ysqlite3 {

class Statement
{
public:
	Statement() = default;
	/**
	 * Associates this object with an SQLite statement.
	 *
	 * @pre either both parameters are `nullptr` or none
	 *
	 * @param[in] stmt the statement
	 * @param[in] db the database
	 */
	Statement(sqlite3_stmt* stmt, sqlite3* db);
	Statement(const Statement& copy) = delete;
	/**
	 * Move-Constructor.
	 *
	 * @post `move` is closed
	 *
	 * @param[in,out] move the other statement
	 */
	Statement(Statement&& move) noexcept;
	/**
	 * Destructor.
	 *
	 * @post the statement is closed
	 */
	virtual ~Statement();
	/**
	 * Clears all set bindings. If this statement is closed, this is a noop.
	 */
	void clear_bindings();
	/**
	 * Evaluates the SQL statement. The returned data is discarded.
	 *
	 * @pre the statement is not closed
	 * @post the statement is reset
	 *
	 * @exception std::system_error if the step could not be executed properly
	 */
	void finish();
	/**
	 * Resets the statement. If this statement is closed, this is a noop.
	 *
	 * @post bindings will remain unchanged
	 *
	 * @exception std::system_error if the reset could not be completed
	 */
	void reset();
	/**
	 * Closes the statement. Closing a closed statement has no effect.
	 *
	 * @post the database is closed
	 *
	 * @exception std::system_error if closing failed for some reason
	 */
	void close();
	bool is_open() const noexcept;
	/**
	 * Evaluates the SQL statement step by step and return the data. This function must be called one or more
	 * times.
	 *
	 * @pre the statement is not closed
	 *
	 * @return the fetched results
	 * @throw exception::database_exception if the step could not be executed properly
	 */
	Results step();
	Statement& bind_reference(Index index, std::string_view value);
	Statement& bind_reference(Index index, std::string&& value) = delete;
	Statement& bind_reference(Index index, std::span<const std::byte> blob);
	Statement& bind(Index index, std::string_view value);
	Statement& bind(Index index, std::nullptr_t /* value */);
	Statement& bind(Index index, double value);
	template<typename Type>
	Statement& bind(Index index, const std::optional<Type>& value)
	{
		if (value.has_value()) {
			return bind(std::move(index), *value);
		}
		return bind(std::move(index), nullptr);
	}
	template<typename Type>
	Statement& bind_reference(Index index, const std::optional<Type>& value)
	{
		if (value.has_value()) {
			return bind_reference(std::move(index), *value);
		}
		return bind(std::move(index), nullptr);
	}
	template<typename Type>
	std::enable_if_t<std::is_integral_v<Type> && !std::is_same_v<Type, bool>, Statement&> bind(Index index,
	                                                                                           Type value)
	{
		if (value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max()) {
			return _bind(index, sqlite3_bind_int, static_cast<int>(value));
		}
		return _bind(index, sqlite3_bind_int64, numeric_cast<sqlite3_int64>(value));
	}
	Statement& bind_zeros(Index index, sqlite3_uint64 size);
	/// Returns whether the statement makes no direct changes to the database.
	bool readonly();
	/**
	 * Returns the parameter count that can be bound.
	 *
	 * @pre the statement is not closed
	 *
	 * @return the parameter count
	 */
	int parameter_count();
	/**
	 * Returns the column count of the result rows.
	 *
	 * @pre the statement is not closed
	 *
	 * @return the column count
	 */
	int column_count();
	/**
	 * Returns the selected column names of a SELECT statement. If this is no SELECT statement, no columns are
	 * returned.
	 *
	 * @pre the statement is not closed
	 *
	 * @return the column name
	 */
	std::vector<std::string> columns();
	/**
	 * Returns the SQLite3 statment handle.
	 *
	 * @return the statment handle or `nullptr` if the statment is closed
	 */
	sqlite3_stmt* handle() noexcept;
	/**
	 * Returns the SQLite3 statment handle.
	 *
	 * @return the statment handle or `nullptr` if the statment is closed
	 */
	const sqlite3_stmt* handle() const noexcept;
	/**
	 * Returns the SQLite statemet handle. This statement object will be marked as closed, but the handle will
	 * remain open.
	 *
	 * @post the statement is closed
	 *
	 * @return the handle
	 */
	sqlite3_stmt* release() noexcept;
	Statement& operator=(const Statement& copy) = delete;
	Statement& operator=(Statement&& move) noexcept;

private:
	sqlite3_stmt* _statement = nullptr;
	sqlite3* _database       = nullptr;

	/**
	 * Returns the integer index of the parameter.
	 *
	 * @exception std::system_error if the parameter name is unknown or the index is out of range
	 * @param index the parameter
	 * @return the integer index
	 */
	int _to_parameter_index(Index index);
	template<typename Binder, typename... Args>
	Statement& _bind(const Index& index, Binder&& binder, Args&&... args)
	{
		if (!is_open()) {
			throw std::system_error{ Error::statement_is_closed };
		} else if (const auto ec = binder(_statement, _to_parameter_index(index), std::forward<Args>(args)...)) {
			throw std::system_error{ static_cast<SQLite3Error>(ec) };
		}
		return *this;
	}
};

} // namespace ysqlite3
