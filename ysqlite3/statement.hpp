#pragma once

#include "sqlite3.h"

namespace ysqlite3 {

class statement
{
public:
    /**
     Associates this object with an SQLite statement.

     @param[in] stmt the statement
    */
    statement(sqlite3_stmt* stmt) noexcept;
    virtual ~statement();
    /**
     Clears all set bindings.

     @throws exception::closed_statement_exception if the statement is closed
    */
    void clear_bindings();
    /**
     Evaluates the SQL statement. The returned data is discarded.

     @throws see step()
    */
    void finish();
    /**
     Closes the statement. Closing a closed statement has no effect.

     @throws exception::database_exception if closing failed for some reason
    */
    void close();
    /**
     Returns whether the statement is closed or not.

     @returns `true` if the statement is closed, otherwise `false`
    */
    bool closed() noexcept;
    /**
     Evaluates the SQL statement step by step and returns the data. This function must be called one or more times.

     @returns `true` if a row was fetched or `false` if no more rows are available, i.e. the statement is finished
     @throws exception::closed_statement_exception if the statement is closed
     @throws exception::database_exception if the step could not be executed properly
    */
    bool step();
    void bind();
    template<typename T>
    void bind(T&& value)
    {}

private:
    sqlite3_stmt* _statement;

    void _assert_statement();
};

} // namespace ysqlite3