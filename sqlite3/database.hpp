#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include "config.hpp"
#include "handle.hpp"
#include "exception.hpp"


namespace sqlite3
{

enum SQLITE3_OPEN
{
	SO_READONLY = 0x00000001,
	SO_READWRITE = 0x00000002,
	SO_CREATE = 0x00000004,
	SO_URI = 0x00000040,
	SO_MEMORY = 0x00000080,
	SO_NOMUTEX = 0x00008000,
	SO_FULLMUTEX = 0x00010000,
	SO_SHAREDCACHE = 0x00020000,
	SO_PRIVATECACHE = 0x00040000,
};

class statement;

class database
{
public:
	/**
	 * Opens a SQLite3 database connection.
	 *
	 * @since 1.0.0.1
	 * @date 29-Sep-18
	 *
	 * @param _filename A UTF-8 encoded file.
	 * @param _mode The flags the database should be opened with. See @ref SQLITE3_OPEN.
	 *
	 * @throws database_error When the specified file couldn't be opened.
	*/
	SQLITE3_API database(const char * _filename, int _mode = SO_READWRITE | SO_CREATE);
	/**
	 * Destructor.
	 *
	 * @since 1.0.0.1
	 * @date 29-Sep-18
	*/
	virtual ~database() noexcept = default;
	/**
	 * Executes the SQL query.
	 *
	 * @since 1.0.0.2
	 * @date 29-Sep-18
	 *
	 * @param _sql The UTF-8 SQL query.
	 *
	 * @throws programming_error If the query if invalid.
	*/
	SQLITE3_API void execute(const char * _sql);
	SQLITE3_API void enable_foreign_keys(bool _enable);
	SQLITE3_API void begin_transaction();
	SQLITE3_API void commit();
	SQLITE3_API void rollback();
	SQLITE3_API bool is_readonly() const;
	SQLITE3_API long long last_insert_rowid() const noexcept;

protected:
	friend statement;

	/** Holds all relevant information about the underlying sqlite3 connection. */
	std::shared_ptr<handle> _connection;


	database();
};

}