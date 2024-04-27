#include "statement.hpp"

#include "error.hpp"
#include "finally.hpp"

namespace ysqlite3 {

Statement::Statement(sqlite3_stmt* stmt, sqlite3* db)
{
	if (static_cast<bool>(stmt) != static_cast<bool>(db)) {
		throw std::system_error{ Error::bad_arguments };
	}
	_statement = stmt;
	_database  = db;
}

Statement::Statement(Statement&& move) noexcept
{
	std::swap(_statement, move._statement);
}

Statement::~Statement()
{
	try {
		close();
	} catch (...) {
	}
}

void Statement::clear_bindings()
{
	if (is_open()) {
		sqlite3_clear_bindings(_statement);
	}
}

void Statement::finish()
{
	if (!is_open()) {
		throw std::system_error{ Error::statement_is_closed };
	}

	// Step until SQLITE_DONE is returned.
	while (true) {
		const int ec = sqlite3_step(_statement);
		if (ec != SQLITE_ROW) {
			sqlite3_reset(_statement);
			if (ec == SQLITE_DONE) {
				return;
			}
			throw std::system_error{ static_cast<SQLite3Error>(ec) };
		}
	}
}

void Statement::reset()
{
	if (is_open()) {
		if (const int ec = sqlite3_reset(_statement)) {
			throw std::system_error{ static_cast<SQLite3Error>(ec) };
		}
	}
}

void Statement::close()
{
	const int ec = sqlite3_finalize(_statement);
	_statement   = nullptr;
	if (ec) {
		throw std::system_error{ static_cast<SQLite3Error>(ec) };
	}
}

bool Statement::is_open() const noexcept
{
	return _statement != nullptr;
}

Results Statement::step()
{
	if (!is_open()) {
		throw std::system_error{ Error::statement_is_closed };
	}

	const int ec = sqlite3_step(_statement);
	if (ec == SQLITE_ROW) {
		return { _statement, _database };
	}
	const auto _ = finally([this] { sqlite3_reset(_statement); });
	if (ec == SQLITE_DONE) {
		return {};
	}
	throw std::system_error{ static_cast<SQLite3Error>(ec), sqlite3_errmsg(_database) };
}

Statement& Statement::bind_reference(Index index, std::string_view value)
{
	return _bind(index, sqlite3_bind_text, value.data(), numeric_cast<int>(value.size()), SQLITE_STATIC);
}

Statement& Statement::bind_reference(Index index, Span<const std::uint8_t*> blob)
{
	return _bind(index, sqlite3_bind_blob64, blob.begin(), numeric_cast<int>(blob.size()), SQLITE_STATIC);
}

Statement& Statement::bind(Index index, std::string_view value)
{
	return _bind(index, sqlite3_bind_text, value.data(), numeric_cast<int>(value.size()), SQLITE_TRANSIENT);
}

Statement& Statement::bind(Index index, std::nullptr_t /* value */)
{
	return _bind(index, sqlite3_bind_null);
}

Statement& Statement::bind(Index index, double value)
{
	return _bind(index, sqlite3_bind_double, value);
}

Statement& Statement::bind_zeros(Index index, sqlite3_uint64 size)
{
	if (size <= std::numeric_limits<int>::max()) {
		return _bind(index, sqlite3_bind_zeroblob, static_cast<int>(size));
	}
	return _bind(index, sqlite3_bind_zeroblob64, size);
}

bool Statement::readonly()
{
	if (!is_open()) {
		throw std::system_error{ Error::statement_is_closed };
	}
	return sqlite3_stmt_readonly(_statement);
}

int Statement::parameter_count()
{
	if (!is_open()) {
		throw std::system_error{ Error::statement_is_closed };
	}
	return sqlite3_bind_parameter_count(_statement);
}

int Statement::column_count()
{
	if (!is_open()) {
		throw std::system_error{ Error::statement_is_closed };
	}
	return sqlite3_column_count(_statement);
}

std::vector<std::string> Statement::columns()
{
	if (!is_open()) {
		throw std::system_error{ Error::statement_is_closed };
	}
	std::vector<std::string> columns;
	for (int i = 0, c = sqlite3_column_count(_statement); i < c; ++i) {
		columns.push_back(sqlite3_column_name(_statement, i));
	}
	return columns;
}

sqlite3_stmt* Statement::handle() noexcept
{
	return _statement;
}

const sqlite3_stmt* Statement::handle() const noexcept
{
	return _statement;
}

sqlite3_stmt* Statement::release() noexcept
{
	auto tmp   = _statement;
	_statement = nullptr;
	return tmp;
}

Statement& Statement::operator=(Statement&& move) noexcept
{
	std::swap(_statement, move._statement);
	return *this;
}

int Statement::_to_parameter_index(Index index)
{
	if (index.name) {
		const auto idx = sqlite3_bind_parameter_index(_statement, index.name);
		if (!idx) {
			throw std::system_error{ Error::unknown_parameter };
		}
		return idx;
	} else if (index.value < 0 || index.value >= sqlite3_bind_parameter_count(_statement)) {
		throw std::system_error{ Error::parameter_out_of_range };
	}
	return index.value + 1;
}

} // namespace ysqlite3
