#ifndef YSQLITE3_STATEMENT_HPP_
#define YSQLITE3_STATEMENT_HPP_

#include "exception/database_exception.hpp"
#include "exception/parameter_exception.hpp"
#include "sqlite3.h"

#include <cstddef>
#include <gsl/gsl>
#include <string>
#include <utility>

namespace ysqlite3 {

class statement
{
public:
	class index
	{
	public:
		index(int index);
		index(gsl::czstring<> name);

	private:
		friend statement;

		int _index        = 0;
		const char* _name = nullptr;
	};

	/**
	 * Associates this object with an SQLite statement.
	 *
	 * @param[in] stmt the statement
	 */
	statement(sqlite3_stmt* stmt) noexcept;
	statement(const statement& copy) = delete;
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
	 * Clears all set bindings.
	 *
	 * @pre the statement is not closed
	 */
	void clear_bindings();
	/**
	 * Evaluates the SQL statement. The returned data is discarded.
	 *
	 * @pre the statement is not closed
	 * @post the statement is reset
	 *
	 * @throw exception::database_exception if the step could not be executed properly
	 */
	void finish();
	/**
	 * Resets the statement.
	 *
	 * @pre the statement is not closed
	 * @post bindings will remain unchanged
	 *
	 * @throw exception::database_exception if the reset could not be completed
	 */
	void reset();
	/**
	 * Closes the statement. Closing a closed statement has no effect.
	 *
	 * @post the database is closed
	 *
	 * @throw exception::database_exception if closing failed for some reason
	 */
	void close();
	/**
	 * Returns whether the statement is closed or not.
	 *
	 * @returns `true` if the statement is closed, otherwise `false`
	 */
	bool closed() noexcept;
	/**
	 * Evaluates the SQL statement step by step and returns the data. This function must be called one or more
	 * times.
	 *
	 * @pre the statement is not closed
	 *
	 * @returns `true` if a row was fetched or `false` if no more rows are available, i.e. the statement is
	 * finished
	 * @throw exception::database_exception if the step could not be executed properly
	 */
	bool step();
	statement& bind_reference(index index, gsl::czstring<> value);
	statement& bind_reference(index index, const std::string& value);
	statement& bind(index index, gsl::czstring<> value);
	statement& bind(index index, const std::string& value);
	statement& bind(index index, std::nullptr_t);
	statement& bind(index index, double value);
	statement& bind(index index, int value);
	statement& bind(index index, sqlite3_int64 value);
	statement& bind_zeros(index index, int size);
	statement& bind_zeros64(index index, sqlite3_uint64 size);
	/**
	 * Returns whether the statement makes no direct changes to the database.
	 *
	 * @pre the statement is not closed
	 *
	 * @returns `true` if read-only, otherwise `false`
	 */
	bool readonly();
	/**
	 * Returns the parameter count that can be bound.
	 *
	 * @pre the statement is not closed
	 *
	 * @returns the parameter count
	 */
	int parameter_count();
	/**
	 * Returns the column count of the result rows.
	 *
	 * @pre the statement is not closed
	 *
	 * @returns the column count
	 */
	int column_count();
	/**
	 * Returns the SQLite3 statment handle.
	 *
	 * @returns the statment handle or `nullptr` if the statment is closed
	 */
	sqlite3_stmt* handle() noexcept;
	/**
	 * Returns the SQLite3 statment handle.
	 *
	 * @returns the statment handle or `nullptr` if the statment is closed
	 */
	const sqlite3_stmt* handle() const noexcept;
	/**
	 * Returns the SQLite statemet handle. This statement object will be marked as closed, but the handle will
	 * remain open.
	 *
	 * @post the statement is closed
	 *
	 * @returns the handle
	 */
	gsl::owner<sqlite3_stmt*> release() noexcept;
	statement& operator=(statement&& move) noexcept;

private:
	sqlite3_stmt* _statement = nullptr;

	int _to_column_index(const index& index);
	template<typename Binder, typename... Args>
	statement& _bind(const index& index, Binder&& binder, Args&&... args)
	{
		Expects(!closed());

		auto error = binder(_statement, _to_column_index(index), std::forward<Args>(args)...);

		if (error != SQLITE_OK) {
			YSQLITE_THROW(exception::database_exception, error, "could not bind value");
		}

		return *this;
	}
};

} // namespace ysqlite3

#endif