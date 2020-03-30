#include "statement.hpp"

#include <limits>

using namespace ysqlite3;

statement::statement(sqlite3_stmt* stmt, sqlite3* db)
{
	Expects(static_cast<bool>(stmt) == static_cast<bool>(db));

	_statement = stmt;
	_database  = db;
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

bool statement::closed() const noexcept
{
	return !_statement;
}

results statement::step()
{
	Expects(!closed());

	auto error = sqlite3_step(_statement);

	if (error == SQLITE_ROW) {
		return { _statement, _database };
	}

	auto _ = gsl::finally([this] { sqlite3_reset(_statement); });

	if (error == SQLITE_DONE) {
		return {};
	}

	YSQLITE_THROW(exception::database_exception, error, "failed to step");
}

statement& statement::bind_reference(index index, gsl::czstring<> value)
{
	return _bind(index, sqlite3_bind_text, value, -1, SQLITE_STATIC);
}

statement& statement::bind_reference(index index, const std::string& value)
{
	return _bind(index, sqlite3_bind_text, value.c_str(), value.length(), SQLITE_STATIC);
}

statement& statement::bind(index index, gsl::czstring<> value)
{
	return _bind(index, sqlite3_bind_text, value, -1, SQLITE_TRANSIENT);
}

statement& statement::bind(index index, const std::string& value)
{
	return _bind(index, sqlite3_bind_text, value.c_str(), value.length(), SQLITE_TRANSIENT);
}

statement& statement::bind(index index, std::nullptr_t)
{
	return _bind(index, sqlite3_bind_null);
}

statement& statement::bind(index index, double value)
{
	return _bind(index, sqlite3_bind_double, value);
}

statement& statement::bind(index index, sqlite3_int64 value)
{
	if (value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max()) {
		return _bind(index, sqlite3_bind_int, value);
	}

	return _bind(index, sqlite3_bind_int64, value);
}

statement& statement::bind_zeros(index index, sqlite3_uint64 size)
{
	if (size <= std::numeric_limits<int>::max()) {
		return _bind(index, sqlite3_bind_zeroblob, static_cast<int>(size));
	}

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

std::vector<std::string> statement::columns()
{
	Expects(!closed());

	std::vector<std::string> columns;

	for (int i = 0, c = sqlite3_column_count(_statement); i < c; ++i) {
		columns.push_back(sqlite3_column_name(_statement, i));
	}

	return columns;
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

statement& statement::operator=(statement&& move) noexcept
{
	std::swap(_statement, move._statement);

	return *this;
}

int statement::_to_parameter_index(index index)
{
	if (index.name) {
		auto i = sqlite3_bind_parameter_index(_statement, index.name);

		if (!i) {
			YSQLITE_THROW(exception::parameter_exception, "unkown parameter name");
		}

		return i;
	} else if (index.value < 1 || index.value > sqlite3_bind_parameter_count(_statement)) {
		YSQLITE_THROW(exception::parameter_exception, "parameter index out of range");
	}

	return index.value;
}
