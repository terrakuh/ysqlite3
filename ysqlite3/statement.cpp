#include "statement.hpp"

#include "error.hpp"
#include "finally.hpp"

#include <limits>

using namespace ysqlite3;

statement::statement(sqlite3_stmt* stmt, sqlite3* db)
{
	if (static_cast<bool>(stmt) != static_cast<bool>(db)) {
		throw std::system_error{ errc::bad_arguments };
	}

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
	if (is_open()) {
		sqlite3_clear_bindings(_statement);
	}
}

void statement::finish()
{
	if (!is_open()) {
		throw std::system_error{ errc::statement_is_closed };
	}

	// step until SQLITE_DONE is returned
	while (true) {
		const auto ec = sqlite3_step(_statement);

		if (ec != SQLITE_ROW) {
			sqlite3_reset(_statement);

			if (ec == SQLITE_DONE) {
				return;
			}

			throw std::system_error{ static_cast<sqlite3_errc>(ec) };
		}
	}
}

void statement::reset()
{
	if (is_open()) {
		if (const auto ec = sqlite3_reset(_statement)) {
			throw std::system_error{ static_cast<sqlite3_errc>(ec) };
		}
	}
}

void statement::close()
{
	const auto ec = sqlite3_finalize(_statement);
	_statement    = nullptr;

	if (ec) {
		throw std::system_error{ static_cast<sqlite3_errc>(ec) };
	}
}

bool statement::is_open() const noexcept
{
	return _statement;
}

results statement::step()
{
	if (!is_open()) {
		throw std::system_error{ errc::statement_is_closed };
	}

	const auto ec = sqlite3_step(_statement);

	if (ec == SQLITE_ROW) {
		return { _statement, _database };
	}

	const auto _ = finally([this] { sqlite3_reset(_statement); });

	if (ec == SQLITE_DONE) {
		return {};
	}

	throw std::system_error{ static_cast<sqlite3_errc>(ec) };
}

statement& statement::bind_reference(index index, const char* value)
{
	return _bind(index, sqlite3_bind_text, value, -1, SQLITE_STATIC);
}

statement& statement::bind_reference(index index, const std::string& value)
{
	return _bind(index, sqlite3_bind_text, value.c_str(), value.length(), SQLITE_STATIC);
}

statement& statement::bind(index index, const char* value)
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
	if (!is_open()) {
		throw std::system_error{ errc::statement_is_closed };
	}

	return sqlite3_stmt_readonly(_statement);
}

int statement::parameter_count()
{
	if (!is_open()) {
		throw std::system_error{ errc::statement_is_closed };
	}

	return sqlite3_bind_parameter_count(_statement);
}

int statement::column_count()
{
	if (!is_open()) {
		throw std::system_error{ errc::statement_is_closed };
	}

	return sqlite3_column_count(_statement);
}

std::vector<std::string> statement::columns()
{
	if (!is_open()) {
		throw std::system_error{ errc::statement_is_closed };
	}

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

sqlite3_stmt* statement::release() noexcept
{
	auto tmp   = _statement;
	_statement = nullptr;
	return tmp;
}

statement& statement::operator=(statement&& move) noexcept
{
	std::swap(_statement, move._statement);
	return *this;
}

int statement::_to_parameter_index(index index)
{
	if (index.name) {
		const auto idx = sqlite3_bind_parameter_index(_statement, index.name);

		if (!idx) {
			throw std::system_error{ errc::unknown_parameter };
		}

		return idx;
	} else if (index.value < 1 || index.value > sqlite3_bind_parameter_count(_statement)) {
		throw std::system_error{ errc::parameter_out_of_range };
	}

	return index.value;
}
