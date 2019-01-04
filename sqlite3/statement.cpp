#include "statement.hpp"
#include "sqlite3.h"

#include <limits>


namespace ysqlite3
{

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

void statement::finish()
{
	while (step());
}

void statement::set_sql(const char * _sql)
{
	std::unique_ptr<handle> _new(new handle([](void * _statement) { sqlite3_finalize(reinterpret_cast<sqlite3_stmt*>(_statement)); }));

	if (!prepare(_sql)) {
		throw programming_error(sqlite3_errmsg(_connection->get<sqlite3>()));
	}

	_statement.swap(_new);
}

void statement::bind(parameter_indexer _index)
{
	check_bind_result(sqlite3_bind_null(_statement->get<sqlite3_stmt>(), _index.index(this)));
}

void statement::bind(parameter_indexer _index, long long _value)
{
	if (_value >= std::numeric_limits<int>::min() && _value <= std::numeric_limits<int>::max()) {
		check_bind_result(sqlite3_bind_int(_statement->get<sqlite3_stmt>(), _index.index(this), static_cast<int>(_value)));
	} else {
		check_bind_result(sqlite3_bind_int64(_statement->get<sqlite3_stmt>(), _index.index(this), _value));
	}
}

void statement::bind(parameter_indexer _index, double _value)
{
	check_bind_result(sqlite3_bind_double(_statement->get<sqlite3_stmt>(), _index.index(this), _value));
}

void statement::bind(parameter_indexer _index, const std::string & _text)
{
	bind(_index, _text.c_str(), _text.size());
}

void statement::bind(parameter_indexer _index, const char * _text, int _size)
{
	bind_text(_index, _text, _size, SQLITE_TRANSIENT);
}

void statement::bind(parameter_indexer _index, const void * _blob, int _size)
{
	bind_blob(_index, _blob, _size, SQLITE_TRANSIENT);
}

void statement::bind_reference(parameter_indexer _index, const std::string & _text)
{
	bind_reference(_index, _text.c_str(), _text.size());
}

void statement::bind_reference(parameter_indexer _index, const char * _text, int _size)
{
	bind_text(_index, _text, _size, SQLITE_STATIC);
}

void statement::bind_reference(parameter_indexer _index, const void * _blob, int _size)
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

bool statement::is_null(int _index)
{
	return sqlite3_column_blob(_statement->get<sqlite3_stmt>(), _index) == nullptr;
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

long long statement::get_int(int _index)
{
	return sqlite3_column_int64(_statement->get<sqlite3_stmt>(), _index);
}

double statement::get_double(int _index)
{
	return sqlite3_column_double(_statement->get<sqlite3_stmt>(), _index);
}

std::string statement::get_string(int _index)
{
	auto _text = get_text(_index);

	return std::string(_text.first, _text.second);
}

std::pair<const char*, int> statement::get_text(int _index)
{
	return { reinterpret_cast<const char*>(sqlite3_column_text(_statement->get<sqlite3_stmt>(), _index)), sqlite3_column_bytes(_statement->get<sqlite3_stmt>(), _index) };
}

std::pair<const void*, int> statement::get_blob(int _index)
{
	return { reinterpret_cast<const char*>(sqlite3_column_blob(_statement->get<sqlite3_stmt>(), _index)), sqlite3_column_bytes(_statement->get<sqlite3_stmt>(), _index) };
}

bool statement::prepare(const char * _sql)
{
	return sqlite3_prepare_v2(_connection->get<sqlite3>(), _sql, -1, &_statement->get<sqlite3_stmt>(), nullptr) == SQLITE_OK;
}

void statement::bind_text(parameter_indexer _index, const char * _text, int _size, void(*_destructor)(void *))
{
	check_bind_result(sqlite3_bind_text(_statement->get<sqlite3_stmt>(), _index.index(this), _text, _size, _destructor));
}

void statement::bind_blob(parameter_indexer _index, const void * _blob, int _size, void(*_destructor)(void *))
{
	if (_blob) {
		check_bind_result(sqlite3_bind_blob(_statement->get<sqlite3_stmt>(), _index.index(this), _blob, _size, _destructor));
	} // Bind zero blob
	else {
		check_bind_result(sqlite3_bind_zeroblob(_statement->get<sqlite3_stmt>(), _index.index(this), _size));
	}
}

void statement::check_bind_result(int _result)
{
	if (_result != SQLITE_OK) {
		throw programming_error(sqlite3_errmsg(_connection->get<sqlite3>()));
	}
}

}