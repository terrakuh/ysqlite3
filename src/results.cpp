#include "results.hpp"

#include "exception/parameter_exception.hpp"
#include "exception/sqlite3_exception.hpp"

using namespace ysqlite3;

results::results(sqlite3_stmt* statement, sqlite3* database) noexcept
{
	Expects(static_cast<bool>(statement) == static_cast<bool>(database));

	_statement = statement;
	_database  = database;
}

bool results::is_null(index index)
{
	return type_of(index) == type::null;
}

int results::integer(index index)
{
	Expects(*this);

	return sqlite3_column_int(_statement, _to_column_index(index));
}

sqlite3_int64 results::integer64(index index)
{
	Expects(*this);

	return sqlite3_column_int64(_statement, _to_column_index(index));
}

double results::real(index index)
{
	Expects(*this);

	return sqlite3_column_double(_statement, _to_column_index(index));
}

gsl::span<const gsl::byte> results::blob(index index)
{
	Expects(*this);

	const auto i    = _to_column_index(index);
	const auto old  = sqlite3_errcode(_database);
	const auto blob = static_cast<const gsl::byte*>(sqlite3_column_blob(_statement, i));
	const auto err  = sqlite3_errcode(_database);

	if (!blob && err != SQLITE_OK && err != old) {
		YSQLITE_THROW(exception::sqlite3_exception, err, "failed to fetch blob");
	}

	return { blob, sqlite3_column_bytes(_statement, i) };
}

gsl::czstring<> results::text(index index)
{
	Expects(*this);

	const auto old = sqlite3_errcode(_database);
	const auto str = reinterpret_cast<const char*>(sqlite3_column_text(_statement, _to_column_index(index)));
	const auto err = sqlite3_errcode(_database);

	if (!str && err != SQLITE_OK && err != old) {
		YSQLITE_THROW(exception::sqlite3_exception, err, "failed to fetch text");
	}

	return str;
}

int results::size_of(index index)
{
	Expects(*this);

	const auto old  = sqlite3_errcode(_database);
	const auto size = sqlite3_column_bytes(_statement, _to_column_index(index));
	const auto err  = sqlite3_errcode(_database);

	if (!size && err != SQLITE_OK && err != old) {
		YSQLITE_THROW(exception::sqlite3_exception, err, "failed to fetch size");
	}

	return size;
}

results::type results::type_of(index index)
{
	Expects(*this);

	switch (sqlite3_column_type(_statement, _to_column_index(index))) {
	case SQLITE_INTEGER: return type::integer;
	case SQLITE_FLOAT: return type::real;
	case SQLITE_TEXT: return type::text;
	case SQLITE_BLOB: return type::blob;
	default: return type::null;
	}
}

results::operator bool() const noexcept
{
	return _statement;
}

int results::_to_column_index(index index)
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

		YSQLITE_THROW(exception::parameter_exception, "unkown column name");
	}

	return index.value;
}