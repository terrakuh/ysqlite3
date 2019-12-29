#pragma once

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
		index(int index)
		{
			Expects(index > 0);

			_index = index;
			_name  = nullptr;
		}
		index(gsl::cstring_span<> name)
		{
			Expects(not name.empty());

			_index = 0;
			_name  = name.data();
		}

	private:
		friend statement;

		int _index;
		const char* _name;
	};

	/**
	 Associates this object with an SQLite statement.

	 @param[in] stmt the statement
	*/
	statement(sqlite3_stmt* stmt) noexcept
	{
		_statement = stmt;
	}
	statement(const statement& copy) = delete;
	/**
	 Move-Constructor.

	 @post `move` is guaranteed to be closed

	 @param[in,out] move the other statement
	*/
	statement(statement&& move) noexcept
	{
		_statement      = move._statement;
		move._statement = nullptr;
	}
	/**
	 Destructor. Closes the statement.

	 @see statement::close()
	*/
	virtual ~statement()
	{
		try {
			close();
		} catch (...) {
		}
	}
	/**
	 Clears all set bindings.

	 @pre the statement is not closed
	*/
	void clear_bindings()
	{
		Expects(not closed());

		sqlite3_clear_bindings(_statement);
	}
	/**
	 Evaluates the SQL statement. The returned data is discarded.

	 @pre the statement is not closed
	 @post the statement is reset

	 @throws exception::database_exception if the step could not be executed properly
	*/
	void finish()
	{
		Expects(not closed());

		// step until SQLITE_DONE is returned
		while (true) {
			auto error = sqlite3_step(_statement);

			if (error != SQLITE_ROW) {
				auto _ = gsl::finally([this] { sqlite3_reset(_statement); });

				if (error == SQLITE_DONE) {
					return;
				}

				YSQLITE_THROW(exception::database_exception, error, "failed to step");
			}
		}
	}
	/**
	 Closes the statement. Closing a closed statement has no effect.

	 @post the database is closed

	 @throws exception::database_exception if closing failed for some reason
	*/
	void close()
	{
		auto error = sqlite3_finalize(_statement);

		_statement = nullptr;

		if (error != SQLITE_OK) {
			YSQLITE_THROW(exception::database_exception, error, "failed to close the statement");
		}
	}
	/**
	 Returns whether the statement is closed or not.

	 @returns `true` if the statement is closed, otherwise `false`
	*/
	bool closed() noexcept
	{
		return _statement == nullptr;
	}
	/**
	 Evaluates the SQL statement step by step and returns the data. This function must be called one or more times.

	 @pre the statement is not closed

	 @returns `true` if a row was fetched or `false` if no more rows are available, i.e. the statement is finished
	 @throws exception::database_exception if the step could not be executed properly
	*/
	bool step()
	{
		Expects(not closed());

		auto error = sqlite3_step(_statement);

		if (error == SQLITE_ROW) {
			return true;
		}

		auto _ = gsl::finally([this] { sqlite3_reset(_statement); });

		if (error == SQLITE_DONE) {
			return false;
		}

		YSQLITE_THROW(exception::database_exception, error, "failed to step");
	}
	statement& bind(const index& index, std::nullptr_t)
	{
		return _bind(index, sqlite3_bind_null);
	}
	statement& bind(const index& index, double _value)
	{
		return _bind(index, sqlite3_bind_double, _value);
	}
	statement& bind(const index& index, int _value)
	{
		return _bind(index, sqlite3_bind_int, _value);
	}
	statement& bind64(const index& index, sqlite3_int64 _value)
	{
		return _bind(index, sqlite3_bind_int64, _value);
	}
	statement& bind_zeros(const index& index, int _size)
	{
		return _bind(index, sqlite3_bind_zeroblob, _size);
	}
	statement& bind_zeros64(const index& index, sqlite3_uint64 _size)
	{
		return _bind(index, sqlite3_bind_zeroblob64, _size);
	}
	/**
	 Returns whether the statement makes no direct changes to the database.

	 @pre the statement is not closed

	 @returns `true` if read-only, otherwise `false`
	*/
	bool readonly()
	{
		Expects(not closed());

		return sqlite3_stmt_readonly(_statement);
	}
	/**
	 Returns the parameter count that can be bound.

	 @pre the statement is not closed

	 @returns the parameter count
	*/
	int parameter_count()
	{
		Expects(not closed());

		return sqlite3_bind_parameter_count(_statement);
	}
	/**
	 Returns the column count of the result rows.

	 @pre the statement is not closed

	 @returns the column count
	*/
	int column_count()
	{
		Expects(not closed());

		return sqlite3_column_count(_statement);
	}
	/**
	 Returns the SQLite3 statment handle.

	 @returns the statment handle or `nullptr` if the statment is closed
	*/
	sqlite3_stmt* handle() noexcept
	{
		return _statement;
	}
	/**
	 Returns the SQLite3 statment handle.

	 @returns the statment handle or `nullptr` if the statment is closed
	*/
	const sqlite3_stmt* handle() const noexcept
	{
		return _statement;
	}
	gsl::owner<sqlite3_stmt*> release() noexcept
	{
		auto p = _statement;

		_statement = nullptr;

		return p;
	}

private:
	sqlite3_stmt* _statement;

	int _to_column_index(const index& index)
	{
		if (index._name) {
			auto i = sqlite3_bind_parameter_index(_statement, index._name);

			if (!i) {
				YSQLITE_THROW(exception::parameter_exception, "unkown parameter name");
			}

			return i;
		}

		return index._index;
	}
	template<typename Binder, typename... Args>
	statement& _bind(const index& index, Binder&& binder, Args&&... args)
	{
		Expects(not closed());

		auto error = binder(_statement, _to_column_index(index), std::forward<Args>(args)...);

		if (error != SQLITE_OK) {
			YSQLITE_THROW(exception::database_exception, error, "could not bind value");
		}

		return *this;
	}
};

} // namespace ysqlite3