#pragma once

#include <memory>
#include <string>

#include "exception.hpp"
#include "database.hpp"
#include "handle.hpp"


namespace sqlite3
{

class statement
{
public:
	statement(const char * _sql, database & _database);
	/**
	 * Destructor.
	 *
	 * @since 1.0.0.1
	 * @date 29-Sep-18
	*/
	virtual ~statement() noexcept = default;
	/**
	 * Resets the statement but leaves the bindings as is.
	 *
	 * @since 1.0.0.1
	 * @date 29-Sep-18
	*/
	void reset() noexcept;
	/**
	 * Clears all bindings.
	 *
	 * @since 1.0.0.1
	 * @date 29-Sep-18
	*/
	void clear_bindings() noexcept;
	void execute();
	void finish();
	template<typename Type>
	void bind(const char * _name, Type _value)
	{
		bind(parameter_index(_name), _value);
	}
	void bind(const char * _name, const std::string & _text);
	void bind(const char * _name, const char * _text, size_t _size = 0);
	void bind(const char * _name, const void * _blob, size_t _size);
	void bind(int _index);
	void bind(int _index, int _value);
	void bind(int _index, long long _value);
	void bind(int _index, double _value);
	void bind(int _index, const std::string & _text);
	void bind(int _index, const char * _text, size_t _size = 0);
	void bind(int _index, const void * _blob, size_t _size);
	void bind_reference(const char * _name, const std::string & _text);
	void bind_reference(const char * _name, const char * _text, size_t _size = 0);
	void bind_reference(const char * _name, const void * _blob, size_t _size);
	void bind_reference(int _index, const std::string & _text);
	void bind_reference(int _index, const char * _text, size_t _size = 0);
	void bind_reference(int _index, const void * _blob, size_t _size);
	/**
	 * Executes a step.
	 *
	 * @remarks If this returns false another call to this function will throw an @ref programming_error.
	 *
	 * @since 1.0.0.1
	 * @date 29-Sep-18
	 *
	 * @throws programming_error If the statement hasn't been resettet properly.
	 *
	 * @return true if another row is available, otherwise false.
	*/
	bool step();
	/**
	 * Returns the number of columns in the result set.
	 *
	 * @since 1.0.0.1
	 * @date 29-Sep-18
	 *
	 * @return The number of columns.
	*/
	int column_count() const noexcept;
	/**
	 * Returns the index of the named parameter.
	 *
	 * @since 1.0.0.1
	 * @date 29-Sep-18
	 *
	 * @param _name The UTF-8 name.
	 *
	 * @throws programming_error The named parameter does not exists.
	 *
	 * @return The index.
	*/
	int parameter_index(const char * _name) const;
	/**
	 * Returns a copy of the SQL used to create this statement.
	 *
	 * @since 1.0.0.1
	 * @date 29-Sep-18
	 *
	 * @return The SQL.
	*/
	const char * sql() const noexcept;
	//const char * column_name(int _index) const noexcept;

protected:
	/** The underlying sqlite3 statement. */
	std::unique_ptr<handle> _statement;
	/** Holds all relevant information about the underlying sqlite3 connection. */
	std::shared_ptr<handle> _connection;

	virtual bool prepare(const char * _sql);

private:
	void bind_text(int _index, const char * _text, size_t _size, void(*_destructor)(void*));
	void bind_blob(int _index, const void * _blob, size_t _size, void(*_destructor)(void*));
	void check_bind_result(int _result);
};

}