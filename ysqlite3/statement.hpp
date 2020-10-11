#ifndef YSQLITE3_STATEMENT_HPP_
#define YSQLITE3_STATEMENT_HPP_

#include "results.hpp"
#include "sqlite3.h"
#include "span.hpp"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace ysqlite3 {

class statement
{
public:
	/**
	 * Associates this object with an SQLite statement.
	 *
	 * @pre either both parameters are `nullptr` or none
	 *
	 * @param[in] stmt the statement
	 * @param[in] db the database
	 */
	statement(sqlite3_stmt* stmt, sqlite3* db);
	/**
	 * Move-Constructor.
	 *
	 * @post `move` is closed
	 *
	 * @param[in,out] move the other statement
	 */
	statement(statement&& move) noexcept;
	/**
	 * Destructor.
	 *
	 * @post the statement is closed
	 */
	virtual ~statement();
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
	results step();
	statement& bind_reference(index index, const char* value);
	statement& bind_reference(index index, const std::string& value);
	statement& bind_reference(index index, span<const std::uint8_t*> blob);
	statement& bind(index index, const char* value);
	statement& bind(index index, const std::string& value);
	statement& bind(index index, std::nullptr_t);
	statement& bind(index index, double value);
	statement& bind(index index, sqlite3_int64 value);
	statement& bind_zeros(index index, sqlite3_uint64 size);
	/**
	 * Returns whether the statement makes no direct changes to the database.
	 *
	 * @pre the statement is not closed
	 *
	 * @return `true` if read-only, otherwise `false`
	 */
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
	statement& operator=(statement&& move) noexcept;

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
	int _to_parameter_index(index index);
	template<typename Binder, typename... Args>
	statement& _bind(const index& index, Binder&& binder, Args&&... args)
	{
		if (!is_open()) {
			throw std::system_error{ errc::statement_is_closed };
		} else if (const auto ec =
		               binder(_statement, _to_parameter_index(index), std::forward<Args>(args)...)) {
			throw std::system_error{ static_cast<sqlite3_errc>(ec) };
		}

		return *this;
	}
};

} // namespace ysqlite3

#endif
