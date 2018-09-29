#include "statement.hpp"

#include <limits>


namespace sqlite3
{

#include "sqlite3.h"


statement::statement(const char * _sql, database & _database) : _statement(new handle([](void * _statement) { sqlite3_finalize(reinterpret_cast<sqlite3_stmt*>(_statement)); })), _connection(_database._connection)
{
	if (!prepare(_sql)) {
		throw programming_error(sqlite3_errmsg(_connection->get<sqlite3>()));
	}
}

void statement::reset() noexcept
{
	sqlite3_reset(_statement->get<sqlite3_stmt>());
}

void statement::clear_bindings() noexcept
{
	sqlite3_clear_bindings(_statement->get<sqlite3_stmt>());
}

void statement::execute()
{
	reset();

	finish();
}

void statement::finish()
{
	while (step());
}

void statement::bind(column_index _index)
{
	check_bind_result(sqlite3_bind_null(_statement->get<sqlite3_stmt>(), _index.index(this)));
}

void statement::bind(column_index _index, long long _value)
{
	if (_value >= std::numeric_limits<int>::min() && _value <= std::numeric_limits<int>::max()) {
		check_bind_result(sqlite3_bind_int(_statement->get<sqlite3_stmt>(), _index.index(this), static_cast<int>(_value)));
	} else {
		check_bind_result(sqlite3_bind_int64(_statement->get<sqlite3_stmt>(), _index.index(this), _value));
	}
}

void statement::bind(column_index _index, double _value)
{
	check_bind_result(sqlite3_bind_double(_statement->get<sqlite3_stmt>(), _index.index(this), _value));
}

void statement::bind(column_index _index, const std::string & _text)
{
	bind(_index, _text.c_str(), _text.size());
}

void statement::bind(column_index _index, const char * _text, size_t _size)
{
	bind_text(_index, _text, _size, SQLITE_TRANSIENT);
}

void statement::bind(column_index _index, const void * _blob, size_t _size)
{
	bind_blob(_index, _blob, _size, SQLITE_TRANSIENT);
}

void statement::bind_reference(column_index _index, const std::string & _text)
{
	bind_reference(_index, _text.c_str(), _text.size());
}

void statement::bind_reference(column_index _index, const char * _text, size_t _size)
{
	bind_text(_index, _text, _size, SQLITE_STATIC);
}

void statement::bind_reference(column_index _index, const void * _blob, size_t _size)
{
	bind_blob(_index, _blob, _size, SQLITE_STATIC);
}

bool statement::step()
{
	auto _result = sqlite3_step(_statement->get<sqlite3_stmt>());

	if (_result == SQLITE_ROW) {
		return true;
	} else if (_result == SQLITE_DONE) {
		return false;
	}

	throw programming_error(sqlite3_errmsg(_connection->get<sqlite3>()));
}

int statement::column_count() const noexcept
{
	return sqlite3_column_count(_statement->get<sqlite3_stmt>());
}

int statement::parameter_index(const char * _name) const
{
	auto _index = sqlite3_bind_parameter_index(_statement->get<sqlite3_stmt>(), _name);

	if (!_index) {
		throw programming_error("Unkown parameter.");
	}

	return _index;
}

const char * statement::sql() const noexcept
{
	return sqlite3_sql(_statement->get<sqlite3_stmt>());
}

bool statement::prepare(const char * _sql)
{
	return sqlite3_prepare_v2(_connection->get<sqlite3>(), _sql, -1, &_statement->get<sqlite3_stmt>(), nullptr) == SQLITE_OK;
}

void statement::bind_text(column_index _index, const char * _text, size_t _size, void(*_destructor)(void *))
{
	if (!_size) {
		_size = std::char_traits<char>::length(_text);
	}

	if (_size < std::numeric_limits<int>::max()) {
		check_bind_result(sqlite3_bind_text(_statement->get<sqlite3_stmt>(), _index.index(this), _text, static_cast<int>(_size), _destructor));
	} else {
		check_bind_result(sqlite3_bind_text64(_statement->get<sqlite3_stmt>(), _index.index(this), _text, _size, _destructor, SQLITE_UTF8));
	}
}

void statement::bind_blob(column_index _index, const void * _blob, size_t _size, void(*_destructor)(void *))
{
	if (_blob) {
		if (_size < std::numeric_limits<int>::max()) {
			check_bind_result(sqlite3_bind_blob(_statement->get<sqlite3_stmt>(), _index.index(this), _blob, static_cast<int>(_size), _destructor));
		} else {
			check_bind_result(sqlite3_bind_blob64(_statement->get<sqlite3_stmt>(), _index.index(this), _blob, _size, _destructor));
		}
	} // Bind zero blob
	else {
		if (_size < std::numeric_limits<int>::max()) {
			check_bind_result(sqlite3_bind_zeroblob(_statement->get<sqlite3_stmt>(), _index.index(this), static_cast<int>(_size)));
		} else {
			check_bind_result(sqlite3_bind_zeroblob64(_statement->get<sqlite3_stmt>(), _index.index(this), _size));
		}
	}
}

void statement::check_bind_result(int _result)
{
	if (_result != SQLITE_OK) {
		throw programming_error(sqlite3_errmsg(_connection->get<sqlite3>()));
	}
}

}