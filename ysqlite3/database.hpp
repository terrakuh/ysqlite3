#pragma once

#include "sqlite3.h"

#include <cstddef>
#include <string>

namespace ysqlite3 {

class statement;
class result;

/**
 The database class. This class manages a single database connection.
*/
class database
{
public:
    enum OPEN_FLAGS
    {
        OF_READONLY       = SQLITE_OPEN_READONLY,
        OF_READWRITE      = SQLITE_OPEN_READWRITE,
        OF_CREATE         = SQLITE_OPEN_CREATE,
        OF_DELETEONCLOSE  = SQLITE_OPEN_DELETEONCLOSE,
        OF_EXCLUSIVE      = SQLITE_OPEN_EXCLUSIVE,
        OF_AUTOPROXY      = SQLITE_OPEN_AUTOPROXY,
        OF_URI            = SQLITE_OPEN_URI,
        OF_MEMORY         = SQLITE_OPEN_MEMORY,
        OF_MAIN_DB        = SQLITE_OPEN_MAIN_DB,
        OF_TEMP_DB        = SQLITE_OPEN_TEMP_DB,
        OF_TRANSIENT_DB   = SQLITE_OPEN_TRANSIENT_DB,
        OF_MAIN_JOURNAL   = SQLITE_OPEN_MAIN_JOURNAL,
        OF_TEMP_JOURNAL   = SQLITE_OPEN_TEMP_JOURNAL,
        OF_SUBJOURNAL     = SQLITE_OPEN_SUBJOURNAL,
        OF_MASTER_JOURNAL = SQLITE_OPEN_MASTER_JOURNAL,
        OF_NOMUTEX        = SQLITE_OPEN_NOMUTEX,
        OF_FULLMUTEX      = SQLITE_OPEN_FULLMUTEX,
        OF_SHAREDCACHE    = SQLITE_OPEN_SHAREDCACHE,
        OF_PRIVATECACHE   = SQLITE_OPEN_PRIVATECACHE,
        OF_WAL            = SQLITE_OPEN_WAL
    };

    /**
     Creates a database without a connection.
    */
    database() noexcept;
    /**
     Creates a database and opens the given file.

     @param file the database file
     @throws see database::open()
    */
    database(const std::string& file);
    database(const database& copy) = delete;
    /**
     Move-Constructor. After this operation `move` is guaranteed to be closed.

     @param[in,out] move the other database
    */
    database(database&& move) noexcept;
    /**
     Destructor. Closes the database file if necessary.

     @see force-closing with database::close()
    */
    virtual ~database();

    /**
     Closes this database. Closing a closed database is a noop.

     @param force (opt) if `true` the database is closed when every statement was finalized and/or backups finished
     @throws exception::database_exception if `force == false` and if the database is busy 
    */
    void close(bool force = false);
    /**
     Opens the given file with the specified parameters. If a database is already open, it will be closed.

     @param file the file name
     @param flags (opt) the open flags, by default the file will be opened with `OF_READWRITE | OF_CREATE`; see
     OPEN_FLAGS
     @param vfs (opt) the virtual filesystem that should be used; if empty, the default vfs will be used
     @throws exception::database_exception if the database could not be opened
     @throws see database::close()
    */
    void open(const std::string& file, int flags = OF_READWRITE | OF_CREATE, const std::string& vfs = "");
    /**
     Runs the SQL statement and ignores the result values.

     @param sql zero or more SQL statements
     @returns the amount of returned result rows
     @throws exception::closed_database_exception if the database is closed
     @throws exception::database_exception if a database error occurred
     @throws exception::sql_exception if the SQL statement is invalid
    */
    std::size_t execute(const std::string& sql);
    /**
     Returns the SQLite3 database handle.

     @returns the database handle or `nullptr` if the database is closed
    */
    sqlite3* handle() noexcept;
    /**
     Returns the SQLite3 database handle.

     @returns the database handle or `nullptr` if the database is closed
    */
    const sqlite3* handle() const noexcept;

private:
    sqlite3* _database;

    void _assert_open();
};

} // namespace ysqlite3