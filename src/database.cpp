#include <ysqlite3/database.hpp>
#include <ysqlite3/exception/closed_database_exception.hpp>
#include <ysqlite3/exception/database_exception.hpp>
#include <ysqlite3/exception/sql_exception.hpp>
#include <ysqlite3/scope_exit.hpp>

namespace ysqlite3 {

database::database() noexcept
{
    _database = nullptr;
}

database::database(const std::string& file)
{
    open(file);
}

database::database(database&& move) noexcept
{
    _database      = move._database;
    move._database = nullptr;
}

database::~database()
{
    close(true);
}

void database::close(bool force)
{
    if (_database) {
        if (force) {
            sqlite3_close_v2(_database);
        } else {
            auto error = sqlite3_close(_database);

            if (error != SQLITE_OK) {
                YSQLITE_THROW(exception::database_exception, error, "cloud not close");
            }
        }

        _database = nullptr;
    }
}

void database::open(const std::string& file, int flags, const std::string& vfs)
{
    close();

    auto error = sqlite3_open_v2(file.c_str(), &_database, flags, vfs.empty() ? nullptr : vfs.c_str());

    if (error != SQLITE_OK) {
        _database = nullptr;

        YSQLITE_THROW(exception::database_exception, error, "failed to open");
    }
}

std::size_t database::execute(const std::string& sql)
{
    _assert_open();

    char* message      = nullptr;
    std::size_t result = 0;
    auto error         = sqlite3_exec(
        _database, sql.c_str(),
        [](void* result, int, char**, char**) {
            *static_cast<std::size_t*>(result) += 1;

            return SQLITE_OK;
        },
        &result, &message);

    if (message) {
        auto freeer = at_scope_exit([message] { sqlite3_free(message); });

        YSQLITE_THROW(exception::sql_exception, message);
    } else if (error != SQLITE_OK) {
        YSQLITE_THROW(exception::database_exception, error, "failed to execute sql");
    }

    return result;
}

sqlite3* database::handle() noexcept
{
    return _database;
}

const sqlite3* database::handle() const noexcept
{
    return _database;
}

void database::_assert_open()
{
    if (!_database) {
        YSQLITE_THROW(exception::closed_database_exception, "database is closed");
    }
}

} // namespace ysqlite3