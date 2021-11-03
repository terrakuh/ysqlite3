#include "results.hpp"

using namespace ysqlite3;

Results::Results(sqlite3_stmt* statement, sqlite3* database)
{
	if (static_cast<bool>(statement) != static_cast<bool>(database)) {
		throw std::system_error{ Error::bad_arguments };
	}
	_statement = statement;
	_database  = database;
}

bool Results::is_null(Index index)
{
	return type_of(index) == type::null;
}

sqlite3_int64 Results::integer(Index index)
{
	if (!*this) {
		throw std::system_error{ Error::bad_result };
	}
	return sqlite3_column_int64(_statement, _to_column_index(index));
}

double Results::real(Index index)
{
	if (!*this) {
		throw std::system_error{ Error::bad_result };
	}
	return sqlite3_column_double(_statement, _to_column_index(index));
}

Span<const std::uint8_t*> Results::blob(Index index)
{
	if (!*this) {
		throw std::system_error{ Error::bad_result };
	}

	const auto i    = _to_column_index(index);
	const auto old  = sqlite3_errcode(_database);
	const auto blob = static_cast<const std::uint8_t*>(sqlite3_column_blob(_statement, i));
	const auto ec   = sqlite3_errcode(_database);
	if (!blob && ec != SQLITE_OK && ec != old) {
		throw std::system_error{ static_cast<SQLite3_code>(ec) };
	}
	return { blob, blob + sqlite3_column_bytes(_statement, i) };
}

const char* Results::text(Index index)
{
	if (!*this) {
		throw std::system_error{ Error::bad_result };
	}

	const auto old = sqlite3_errcode(_database);
	const auto str = reinterpret_cast<const char*>(sqlite3_column_text(_statement, _to_column_index(index)));
	const auto ec  = sqlite3_errcode(_database);
	if (!str && ec != SQLITE_OK && ec != old) {
		throw std::system_error{ static_cast<SQLite3_code>(ec) };
	}
	return str;
}

int Results::size_of(Index index)
{
	if (!*this) {
		throw std::system_error{ Error::bad_result };
	}

	const auto old  = sqlite3_errcode(_database);
	const auto size = sqlite3_column_bytes(_statement, _to_column_index(index));
	const auto ec   = sqlite3_errcode(_database);
	if (!size && ec != SQLITE_OK && ec != old) {
		throw std::system_error{ static_cast<SQLite3_code>(ec) };
	}
	return size;
}

Results::type Results::type_of(Index index)
{
	if (!*this) {
		throw std::system_error{ Error::bad_result };
	}

	switch (sqlite3_column_type(_statement, _to_column_index(index))) {
	case SQLITE_INTEGER: return type::integer;
	case SQLITE_FLOAT: return type::real;
	case SQLITE_TEXT: return type::text;
	case SQLITE_BLOB: return type::blob;
	default: return type::null;
	}
}

Results::operator bool() const noexcept
{
	return _statement;
}

int Results::_to_column_index(Index index)
{
	if (index.name) {
		const auto iequals = [](const char* left, const char* right, const std::locale& locale) {
			while (*left && *right) {
				if (std::tolower(*left++) != std::tolower(*right++, locale)) {
					return false;
				}
			}
			return *left == *right;
		};

		for (int i = 0, c = sqlite3_column_count(_statement); i < c; ++i) {
			if (iequals(index.name, sqlite3_column_name(_statement, i), index.locale)) {
				return i;
			}
		}

		throw std::system_error{ Error::unknown_parameter };
	} else if (index.value < 0 || index.value >= sqlite3_column_count(_statement)) {
		throw std::system_error{ Error::parameter_out_of_range };
	}
	return index.value;
}
