#pragma once

#include <memory>
#include <string>
#include <utility>

#include "config.hpp"
#include "exception.hpp"
#include "database.hpp"
#include "handle.hpp"


namespace ysqlite3
{

class statement
{
public:
	class parameter_indexer
	{
	public:
		parameter_indexer(int _index) noexcept
		{
			_type = TYPE::INDEX;
			_value.index = _index;
		}
		parameter_indexer(const char * _name) noexcept
		{
			_type = TYPE::NAME;
			_value.name = _name;
		}
		virtual int index(statement * _this)
		{
			if (_type == TYPE::NAME) {
				return _this->parameter_index(_value.name);
			}

			return _value.index;
		}

	protected:
		enum class TYPE
		{
			INDEX,
			NAME
		} _type;
		union
		{
			int index;
			const char * name;
		} _value;
	};
	
	SQLITE3_API statement(const char * _sql, database & _database);
	statement(statement && _move) = default;
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
	SQLITE3_API void reset() noexcept;
	/**
	 * Clears all bindings.
	 *
	 * @since 1.0.0.1
	 * @date 29-Sep-18
	*/
	SQLITE3_API void clear_bindings() noexcept;
	SQLITE3_API void finish();
	SQLITE3_API void set_sql(const char * _sql);
	template<typename Type>
	typename std::enable_if<std::is_integral<Type>::value>::type bind(parameter_indexer _index, Type _value)
	{
		bind(_index, static_cast<long long>(_value));
	}
	SQLITE3_API void bind(parameter_indexer _index);
	SQLITE3_API void bind(parameter_indexer _index, long long _value);
	SQLITE3_API void bind(parameter_indexer _index, double _value);
	SQLITE3_API void bind(parameter_indexer _index, const std::string & _text);
	SQLITE3_API void bind(parameter_indexer _index, const char * _text, int _size = -1);
	SQLITE3_API void bind(parameter_indexer _index, const void * _blob, int _size);
	SQLITE3_API void bind_reference(parameter_indexer _index, const std::string & _text);
	SQLITE3_API void bind_reference(parameter_indexer _index, const char * _text, int _size = -1);
	SQLITE3_API void bind_reference(parameter_indexer _index, const void * _blob, int _size);
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
	SQLITE3_API bool step();
	/**
	 * Checks whether the column in the result set is null.
	 *
	 * @since 1.0.0.2
	 * @date 29-Sep-18
	 *
	 * @param _index The column index.
	 *
	 * @throws See column_index::index().
	 *
	 * @return true if the set is null.
	*/
	SQLITE3_API bool is_null(int _index);
	/**
	 * Returns the number of columns in the result set.
	 *
	 * @since 1.0.0.1
	 * @date 29-Sep-18
	 *
	 * @return The number of columns.
	*/
	SQLITE3_API int column_count() const noexcept;
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
	SQLITE3_API int parameter_index(const char * _name) const;
	/**
	 * Returns a copy of the SQL used to create this statement.
	 *
	 * @since 1.0.0.1
	 * @date 29-Sep-18
	 *
	 * @return The SQL.
	*/
	SQLITE3_API const char * sql() const noexcept;

	SQLITE3_API long long get_int(int _index);
	SQLITE3_API double get_double(int _index);
	SQLITE3_API std::string get_string(int _index);
	SQLITE3_API std::pair<const char*, int> get_text(int _index);
	SQLITE3_API std::pair<const void*, int> get_blob(int _index);
	statement & operator=(statement && _move) = default;

protected:
	/** The underlying sqlite3 statement. */
	std::unique_ptr<handle> _statement;
	/** Holds all relevant information about the underlying sqlite3 connection. */
	std::shared_ptr<handle> _connection;

	SQLITE3_API virtual bool prepare(const char * _sql);

private:
	SQLITE3_API void bind_text(parameter_indexer _index, const char * _text, int _size, void(*_destructor)(void*));
	SQLITE3_API void bind_blob(parameter_indexer _index, const void * _blob, int _size, void(*_destructor)(void*));
	SQLITE3_API void check_bind_result(int _result);
};

}