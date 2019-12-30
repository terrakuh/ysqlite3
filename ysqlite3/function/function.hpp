#pragma once

#include "../sqlite3.h"

#include <exception>

namespace ysqlite3 {

class database;

namespace function {

class function
{
public:
    enum class TEXT_ENCODING
    {
        UTF_8    = SQLITE_UTF8,
        UTF_16BE = SQLITE_UTF16BE,
        UTF_16LE = SQLITE_UTF16LE
    };

    function(int argc, bool deterministic, bool direct_only, TEXT_ENCODING encoding) noexcept
    {
        _argc  = argc;
        _flags = static_cast<int>(encoding) | (deterministic ? SQLITE_DETERMINISTIC : 0) |
                 (direct_only ? SQLITE_DIRECTONLY : 0);
    }
    virtual ~function() = default;
    static void xfunc(sqlite3_context* context, int argc, sqlite3_value** argv)
    {
        try {
            static_cast<function*>(sqlite3_user_data(context))->run(context, argc, argv);
        } catch (const std::exception& e) {
            sqlite3_result_error(context, e.what(), -1);
        } catch (...) {
            sqlite3_result_error(context, "function threw an exception", -1);
        }
    }

protected:
    virtual void run(sqlite3_context* context, int argc, sqlite3_value** argv) = 0;

private:
    friend database;

    int _argc;
    int _flags;
};

} // namespace function
} // namespace ysqlite3