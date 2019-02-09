#pragma once

#include <memory>
#include <stdexcept>
#include <string>

#include "config.hpp"
#include "handle.hpp"
#include "exception.hpp"
#include "statement.hpp"


namespace ysqlite3
{

class statement;

class database
{
public:
	enum OPEN
	{
		O_READONLY = 0x00000001,
		O_READWRITE = 0x00000002,
		O_CREATE = 0x00000004,
		O_URI = 0x00000040,
		O_MEMORY = 0x00000080,
		O_NOMUTEX = 0x00008000,
		O_FULLMUTEX = 0x00010000,
		O_SHAREDCACHE = 0x00020000,
		O_PRIVATECACHE = 0x00040000,
	};

	enum class JOURNAL_MODE
	{
		DELETE,
		TRUNCATE,
		PERSIST,
		MEMORY,
		WAL,
		OFF
	};

	/**
	 * Opens a SQLite3 database connection.
	 *
	 * @since 1.0.0.1
	 * @date 29-Sep-18
	 *
	 * @param _filename A UTF-8 encoded file.
	 * @param _mode The flags the database should be opened with. See @ref OPEN.
	 *
	 * @throws database_error When the specified file couldn't be opened.
	*/
	SQLITE3_API database(const char * _filename, int _mode = O_READWRITE | O_CREATE);
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
	SQLITE3_API void set_journal_mode(JOURNAL_MODE _mode);
	SQLITE3_API bool is_readonly() const;
	SQLITE3_API long long last_insert_rowid() const noexcept;
	SQLITE3_API statement create_statement(const char * _sql = nullptr);

protected:
	friend statement;

	/** Holds all relevant information about the underlying sqlite3 connection. */
	std::shared_ptr<handle> _connection;


	SQLITE3_API database();
};

}