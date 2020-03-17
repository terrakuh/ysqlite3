#include "statement.hpp"

using namespace ysqlite3;

statement::index::index(int index)
{
	Expects(index > 0);

	_index = index;
}

statement::index::index(gsl::czstring<> name)
{
	Expects(name);

	_name = name;
}

statement::statement(sqlite3_stmt* stmt) noexcept
{
	_statement = stmt;
}

statement::statement(statement&& move) noexcept
{
	std::swap(_statement, move._statement);
}

statement::~statement()
{
	try {
		close();
	} catch (...) {
	}
}

void statement::clear_bindings()
{
	Expects(!closed());

	sqlite3_clear_bindings(_statement);
}

void statement::finish()
{
	Expects(!closed());

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

void statement::reset()
{
	Expects(!closed());

	auto error = sqlite3_reset(_statement);

	if (error != SQLITE_OK) {
		YSQLITE_THROW(exception::database_exception, error, "failed to reset");
	}
}

void statement::close()
{
	auto error = sqlite3_finalize(_statement);

	_statement = nullptr;

	if (error != SQLITE_OK) {
		YSQLITE_THROW(exception::database_exception, error, "failed to close the statement");
	}
}

bool statement::closed() noexcept
{
	return !_statement;
}

bool statement::step()
{
	Expects(!closed());

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

statement& statement::bind(index index, gsl::czstring<> value)
{
	return _bind(index, sqlite3_bind_text, value, -1, nullptr);
}

statement& statement::bind(index index, const std::string& value)
{
	return _bind(index, sqlite3_bind_text, value.c_str(), value.length(), nullptr);
}

statement& statement::bind(index index, std::nullptr_t)
{
	return _bind(index, sqlite3_bind_null);
}

statement& statement::bind(index index, double value)
{
	return _bind(index, sqlite3_bind_double, value);
}

statement& statement::bind(index index, int value)
{
	return _bind(index, sqlite3_bind_int, value);
}

statement& statement::bind64(index index, sqlite3_int64 value)
{
	return _bind(index, sqlite3_bind_int64, value);
}

statement& statement::bind_zeros(index index, int size)
{
	return _bind(index, sqlite3_bind_zeroblob, size);
}

statement& statement::bind_zeros64(index index, sqlite3_uint64 size)
{
	return _bind(index, sqlite3_bind_zeroblob64, size);
}

bool statement::readonly()
{
	Expects(!closed());

	return sqlite3_stmt_readonly(_statement);
}

int statement::parameter_count()
{
	Expects(!closed());

	return sqlite3_bind_parameter_count(_statement);
}

int statement::column_count()
{
	Expects(!closed());

	return sqlite3_column_count(_statement);
}

sqlite3_stmt* statement::handle() noexcept
{
	return _statement;
}

const sqlite3_stmt* statement::handle() const noexcept
{
	return _statement;
}

gsl::owner<sqlite3_stmt*> statement::release() noexcept
{
	auto p = _statement;

	_statement = nullptr;

	return p;
}

int statement::_to_column_index(const index& index)
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
