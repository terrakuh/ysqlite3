#ifndef YSQLITE3_DATABASE_HPP_
#define YSQLITE3_DATABASE_HPP_

#include "exception/database_exception.hpp"
#include "exception/sql_exception.hpp"
#include "function/function.hpp"
#include "sqlite3.h"
#include "statement.hpp"

#include <cstddef>
#include <gsl/gsl>
#include <memory>
#include <type_traits>

namespace ysqlite3 {

/**
 * The database class. This class manages a single database connection.
 */
class database
{
public:
	typedef int open_flag_type;

	enum open_flag
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

	enum class journal_mode
	{
		delete_,
		truncate,
		persist,
		memory,
		wal,
		off
	};

	class transaction
	{
	public:
		transaction(const transaction& copy) = delete;
		/**
		 * Move-Constructor.
		 *
		 * @post move is invalid
		 *
		 * @param[in,out] move the transaction that should be moved
		 */
		transaction(transaction&& move) noexcept;
		/**
		 * Destructor. If the transaction was neither committed nor rollbacked, it will be rollbacked.
		 */
		~transaction();
		/**
		 * Commits the transaction.
		 *
		 * @pre this transaction is open
		 * @post the transaction is not open
		 *
		 * @throw see database::execute()
		 */
		void commit();
		/**
		 * Rollbacks the transaction.
		 *
		 * @pre this transaction is open
		 * @post the transaction is not open
		 *
		 * @throw see database::execute()
		 */
		void rollback();
		/**
		 * Tests whether this transaction is open.
		 *
		 * @returns `true` if it is open, otherwise `false`
		 */
		bool open() const noexcept;
		/**
		 * Tests whether this transaction is open.
		 *
		 * @returns `true` if it is open, otherwise `false`
		 */
		operator bool() const noexcept;

	private:
		friend database;

		database* _db = nullptr;

		transaction(gsl::not_null<database*> db) noexcept;
	};

	/**
	 * Creates a database without a connection.
	 */
	database() = default;
	/**
	 * Creates a database and opens the given file.
	 *
	 * @param file the database file
	 * @throw see database::open()
	 */
	database(gsl::not_null<gsl::czstring<>> file);
	database(const database& copy) = delete;
	/**
	 * Move-Constructor.
	 *
	 * @post `move` is closed
	 *
	 * @param[in,out] move the other database
	 */
	database(database&& move) noexcept;
	/**
	 * Destructor.
	 *
	 * @post the database is closed
	 *
	 * @see force-closing with database::close()
	 */
	virtual ~database();
	/**
	 * Sets the database journaling mode.
	 *
	 * @param mode the journal modus
	 * @throw see database::execute()
	 */
	void set_journal_mode(journal_mode mode);
	/**
	 * Enables or disables foreign key support.
	 *
	 * @param enable whether to enable support or not
	 * @throw see database::execute()
	 */
	void enable_foreign_keys(bool enable = true);
	/**
	 * Closes this database. Closing a closed database is a noop.
	 *
	 * @param force (opt) if `true` the database is closed when every statement was finalized and/or backups finished
	 * @throw exception::database_exception if `force == false` and if the database is busy
	 */
	void close(bool force = false);
	/**
	 * Checks whether the database is closed. This function does not check if the actual database file was forcefully
	 * closed.
	 *
	 * @returns `true` if the database is closed, otherwise `false`
	 */
	bool closed() const noexcept;
	/**
	 * Opens the given file with the specified parameters. If a database is already open, it will be closed.
	 *
	 * @post the database is opened (`closed() == false`)
	 *
	 * @param file the file name
	 * @param flags (opt) the open flags, by default the file will be opened with `open_flag_readwrite |
	 * open_flag_create`; see database::open_flag
	 * @param vfs (opt) the virtual filesystem that should be used; if empty, the default vfs will be used
	 * @throw exception::database_exception if the database could not be opened
	 * @throw see database::close()
	 */
	void open(gsl::not_null<gsl::czstring<>> file, open_flag_type flags = open_flag_readwrite | open_flag_create,
	          gsl::czstring<> vfs = nullptr);
	/**
	 * Registers a normal function which can then be used in SQL statements.
	 * 
	 * @pre the database is openend
	 *
	 * @tparam T the function; must inherit function::function
	 * @tparam Args the arg types for instantiating `T` inplace
	 * @param name the name of the function
	 * @param args the arguments for `T`
	 * @throw exception::database_exception if the function could not be registered
	 * @throw see T()
	 */
	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<function::function, T>::value>::type
	    register_function(gsl::not_null<gsl::czstring<>> name, Args... args)
	{
		Expects(!closed());

		auto instance = std::make_unique<T>(std::forward<Args>(args)...);
		auto error    = sqlite3_create_function_v2(_database, name, instance->_argc, instance->_flags, instance.get(),
                                                function::function::xfunc, nullptr, nullptr,
                                                [](void* instance) { delete static_cast<T*>(instance); });

		if (error != SQLITE_OK) {
			YSQLITE_THROW(exception::database_exception, error, "could not create function");
		}

		instance.release();
	}
	/**
	 * Runs the SQL statement and ignores the result values.
	 *
	 * @pre the database is opened
	 *
	 * @param sql zero or more SQL statements
	 * @returns the amount of returned result rows
	 * @throw exception::database_exception if a database error occurred
	 * @throw exception::sql_exception if the SQL statement is invalid
	 */
	std::size_t execute(gsl::not_null<gsl::czstring<>> sql);
	/**
	 * Creates a prepared statement. Prepared statement can have parameters descibed by any of the following:
	 *
	 * - `?`
	 * - `?NNN`
	 * - `:VVV`
	 * - `@VVV`
	 * - `$VVV`
	 *
	 * @pre the database is opened
	 *
	 * @param sql the SQL statement
	 * @returns the prepared statement
	 * @throw exception::database_exception if the statement could not be created
	 */
	statement prepare_statement(gsl::not_null<gsl::czstring<>> sql);
	/**
	 * Begins a new transaction.
	 *
	 * @returns the transaction object; use this to rollback or commit
	 * @throw see database::execute()
	 */
	transaction begin_transaction();
	/**
	 * Returns the SQLite database file handle. This database will be marked as closed, but the handle will remain open.
	 *
	 * @post the database is closed
	 *
	 * @returns the handle
	 */
	gsl::owner<sqlite3*> release() noexcept;
	/**
	 * Returns the SQLite3 database handle.
	 *
	 * @returns the database handle or `nullptr` if the database is closed
	 */
	sqlite3* handle() noexcept;
	/**
	 * Returns the SQLite3 database handle.
	 *
	 * @returns the database handle or `nullptr` if the database is closed
	 */
	const sqlite3* handle() const noexcept;

private:
	/** the underlying sqlite3 database connection */
	sqlite3* _database = nullptr;
};

} // namespace ysqlite3

#endif