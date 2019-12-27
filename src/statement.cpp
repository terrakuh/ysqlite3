#include <ysqlite3/exception/closed_statement_exception.hpp>
#include <ysqlite3/exception/database_exception.hpp>
#include <ysqlite3/scope_exit.hpp>
#include <ysqlite3/statement.hpp>

namespace ysqlite3 {

statement::statement(sqlite3_stmt* stmt) noexcept
{
    _statement = stmt;
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
    _assert_statement();

    sqlite3_clear_bindings(_statement);
}

void statement::finish()
{
    while (step())
        ;
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
    return _statement == nullptr;
}

bool statement::step()
{
    _assert_statement();

    auto error = sqlite3_step(_statement);

    if (error == SQLITE_ROW) {
        return true;
    }

    auto resetter = at_scope_exit([this] { sqlite3_reset(_statement); });

    if (error == SQLITE_DONE) {
        return false;
    }

    YSQLITE_THROW(exception::database_exception, error, "failed to step");
}

void statement::_assert_statement()
{
    if (closed()) {
        YSQLITE_THROW(exception::closed_statement_exception, "the statement is closed");
    }
}

} // namespace ysqlite3