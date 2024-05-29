#pragma once

#include "error.hpp"
#include "function/function.hpp"
#include "sqlite3.h"
#include "statement.hpp"
#include "transaction.hpp"

#include <cstddef>
#include <memory>

namespace ysqlite3 {

typedef int Open_flags;

using Id = sqlite3_int64;

enum
{
	open_flag_readonly        = SQLITE_OPEN_READONLY,
	open_flag_readwrite       = SQLITE_OPEN_READWRITE,
	open_flag_create          = SQLITE_OPEN_CREATE,
	open_flag_delete_on_close = SQLITE_OPEN_DELETEONCLOSE,
	open_flag_exclusive       = SQLITE_OPEN_EXCLUSIVE,
	open_flag_autoproxy       = SQLITE_OPEN_AUTOPROXY,
	open_flag_uri             = SQLITE_OPEN_URI,
	open_flag_memory          = SQLITE_OPEN_MEMORY,
	open_flag_main_db         = SQLITE_OPEN_MAIN_DB,
	open_flag_temp_db         = SQLITE_OPEN_TEMP_DB,
	open_flag_transient_db    = SQLITE_OPEN_TRANSIENT_DB,
	open_flag_main_journal    = SQLITE_OPEN_MAIN_JOURNAL,
	open_flag_temp_journal    = SQLITE_OPEN_TEMP_JOURNAL,
	open_flag_subjournal      = SQLITE_OPEN_SUBJOURNAL,
	open_flag_master_journal  = SQLITE_OPEN_MASTER_JOURNAL,
	open_flag_no_mutex        = SQLITE_OPEN_NOMUTEX,
	open_flag_full_mutex      = SQLITE_OPEN_FULLMUTEX,
	open_flag_shared_cache    = SQLITE_OPEN_SHAREDCACHE,
	open_flag_private_cache   = SQLITE_OPEN_PRIVATECACHE,
	open_flag_wal             = SQLITE_OPEN_WAL
};

enum class JournalMode
{
	delete_,
	truncate,
	persist,
	memory,
	wal,
	off
};

enum class Text_encoding
{
	utf8    = SQLITE_UTF8,
	utf16be = SQLITE_UTF16BE,
	utf16le = SQLITE_UTF16LE
};



class Database
{
public:
	/// Empty database.
	Database() = default;
	/**
	 * Creates a database and opens the given file.
	 *
	 * @exception see Database::open()
	 * @param file the database file
	 */
	Database(const char* file);
	/// The moved object will be closed.
	Database(Database&& move) noexcept;
	/// Closes the database forcefully if it is open.
	~Database() noexcept;
	/**
	 * Sets the reserved size and vacuums the database if required/desired. The reserved size is required for
	 * the encryption VFS in order to store some custom data like authentication tags or IVs.
	 *
	 * @exception std::system_error - see sqlite3_file_control() and sqlite3_exec() for error codes
	 * @param size the desired size
	 * @param vacuum vacuums if the reserved size was changed; needs to be done if this is an existing
	 * database
	 * @return the old reserved size
	 */
	std::uint8_t set_reserved_size(std::uint8_t size, bool vacuum = true);
	void set_journal_mode(JournalMode mode);
	/// Enables or disables foreign key support.
	void enable_foreign_keys(bool enable = true);
	/**
	 * Closes this database. Closing a closed database is a noop.
	 *
	 * @exception std::system_error - see sqlite3_close() if *force* is `false`
	 * @param force If `true` the database is closed when every statement was finalized and/or backups
	 * finished
	 */
	void close(bool force = false);
	bool is_open() const noexcept;
	/**
	 * Opens the given file with the specified parameters. If a database is already open, it will be closed.
	 *
	 * @exception std::system_error
	 *   - see sqlite3_open_v2()
	 *   - Error::bad_vfs_name if the *vfs* is invalid
	 * @exception see Database::close()
	 * @param file the file name
	 * @param flags (opt) the open flags, by default the file will be opened with `open_flag_readwrite |
	 * open_flag_create`
	 * @param vfs (opt) the virtual filesystem that should be used; if empty, the default vfs will be used
	 */
	void open(const char* file, Open_flags flags = open_flag_readwrite | open_flag_create,
	          const char* vfs = nullptr);
	/**
	 * Registers a normal function which can then be used in SQL statements.
	 *
	 * @exception std::system_error
	 *   - Error::database_is_closed
	 *   - Error::null_function
	 *   - see sqlite3_create_function_v2()
	 * @param name the name of the function
	 * @param function the function
	 */
	void register_function(const char* name, std::unique_ptr<function::Function> function);
	/**
	 * Registers a normal function which can then be used in SQL statements.
	 *
	 * @exception std::system_error
	 *   - Error::database_is_closed
	 *   - Error::null_function
	 *   - see sqlite3_create_function_v2()
	 * @param name the name of the function
	 * @param function the function
	 * @param encoding the desired text encoding
	 * @param deterministic whether the function is deterministic or not
	 * @see more information can be found on the
	 * [SQLite-page](https://www.sqlite.org/c3ref/create_function.html)
	 */
	void register_function(const char* name, void (*function)(sqlite3_context*, int, sqlite3_value**),
	                       function::text_encoding encoding, bool deterministic = true,
	                       bool direct_only = false);
	/**
	 * Runs the SQL statement and ignores the result values.
	 *
	 * @pre the database is opened
	 *
	 * @param sql zero or more SQL statements
	 * @return the amount of returned result rows
	 * @exception exception::database_exception if a database error occurred
	 * @e exception::sql_exception if the SQL statement is invalid
	 */
	std::size_t execute(const char* sql);
	/// Returns the rowid of the last insert statement of this connection or 0 if no insert was made.
	sqlite3_int64 last_insert_rowid() const noexcept;
	/**
	 * Creates a prepared statement. Prepared statement can have parameters described by any of the following:
	 *
	 * - `?`
	 * - `?NNN`
	 * - `:VVV`
	 * - `@VVV`
	 * - `$VVV`
	 *
	 * @pre the database is opened
	 *
	 * @exception std::system_error if the statement could not be created
	 * @param sql the SQL statement
	 * @return the prepared statement
	 */
	[[nodiscard]] Statement prepare_statement(std::string_view sql);
	[[nodiscard]] Transaction begin_transaction();
	/**
	 * Returns the SQLite database file handle. This database will be marked as closed, but the handle will
	 * remain open.
	 *
	 * @post the database is closed
	 *
	 * @return the handle or `nullptr`
	 */
	sqlite3* release_handle() noexcept;
	/// The SQLite3 database handle or `nullptr` if the database is closed.
	sqlite3* handle() noexcept;
	/// The SQLite3 database handle or `nullptr` if the database is closed.
	const sqlite3* handle() const noexcept;
	Database& operator=(Database&& move) noexcept;

private:
	friend Transaction;

	/// Underlying sqlite3 database handle.
	sqlite3* _database = nullptr;

	template<typename Functor>
	static void _functor_forwarder(sqlite3_context* context, int argc, sqlite3_value** argv)
	{
		(*static_cast<Functor*>(sqlite3_user_data(context)))(context, argc, argv);
	}
};

} // namespace ysqlite3
