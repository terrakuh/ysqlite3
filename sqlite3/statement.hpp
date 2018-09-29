#pragma once

#include <memory>
#include <string>
#include <utility>

#include "exception.hpp"
#include "database.hpp"
#include "handle.hpp"


namespace sqlite3
{

class statement
{
public:
	class column_index
	{
	public:
		column_index(int _index) noexcept
		{
			_type = TYPE::INDEX;
			_value.index = _index;
		}
		column_index(const char * _name) noexcept
		{
			_type = TYPE::NAME;
			_value.name = _name;
		}
		int index(statement * _this)
		{
			if (_type == TYPE::NAME) {
				return _this->parameter_index(_value.name);
			}

			return _value.index;
		}

	private:
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
	typename std::enable_if<std::is_integral<Type>::value>::type bind(column_index _index, Type _value)
	{
		bind(_index, static_cast<long long>(_value));
	}
	void bind(column_index _index);
	void bind(column_index _index, long long _value);
	void bind(column_index _index, double _value);
	void bind(column_index _index, const std::string & _text);
	void bind(column_index _index, const char * _text, size_t _size = 0);
	void bind(column_index _index, const void * _blob, size_t _size);
	void bind_reference(column_index _index, const std::string & _text);
	void bind_reference(column_index _index, const char * _text, size_t _size = 0);
	void bind_reference(column_index _index, const void * _blob, size_t _size);
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
	bool is_null(int _index);
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

	long long get_int(int _index);
	double get_double(int _index);
	std::string get_string(int _index);
	std::pair<const char*, size_t> get_text(int _index);
	std::pair<const void*, size_t> get_blob(int _index);

protected:
	/** The underlying sqlite3 statement. */
	std::unique_ptr<handle> _statement;
	/** Holds all relevant information about the underlying sqlite3 connection. */
	std::shared_ptr<handle> _connection;

	virtual bool prepare(const char * _sql);

private:
	void bind_text(column_index _index, const char * _text, size_t _size, void(*_destructor)(void*));
	void bind_blob(column_index _index, const void * _blob, size_t _size, void(*_destructor)(void*));
	void check_bind_result(int _result);
};

}