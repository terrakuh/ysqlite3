#pragma once

#include "exception/database_exception.hpp"
#include "exception/sql_exception.hpp"
#include "function/function.hpp"
#include "sqlite3.h"
#include "statement.hpp"

#include <cstddef>
#include <gsl/gsl>
#include <string>
#include <type_traits>
#include <utility>

namespace ysqlite3 {

/**
 The database class. This class manages a single database connection.
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
		 Move-Constructor.

		 @post move is invalid

		 @param[in,out] move the transaction that should be moved
		*/
		transaction(transaction&& move) noexcept
		{
			_db      = move._db;
			move._db = nullptr;
		}
		/**
		 Destructor. If the transaction was neither committed nor rollbacked, it will be rollbacked.
		*/
		~transaction()
		{
			if (_db) {
				rollback();
			}
		}
		/**
		 Commits the transaction.

		 @pre this transaction is valid
		 @post the transaction is not valid

		 @throws see database::execute()
		*/
		void commit()
		{
			Expects(_db != nullptr);

			_db->execute("COMMIT;");

			_db = nullptr;
		}
		/**
		 Rollbacks the transaction.

		 @pre this transaction is valid
		 @post the transaction is not valid

		 @throws see database::execute()
		*/
		void rollback()
		{
			Expects(_db != nullptr);

			_db->execute("ROLLBACK;");

			_db = nullptr;
		}
		/**
		 Tests whether this transaction is valid.

		 @returns `true` if it is valid, otherwise `false`
		*/
		operator bool() const noexcept
		{
			return _db != nullptr;
		}

	private:
		friend database;

		database* _db;

		transaction(gsl::not_null<database*> db) noexcept
		{
			_db = db;
		}
	};

	/**
	 Creates a database without a connection.
	*/
	database() noexcept
	{
		_database = nullptr;
	}
	/**
	 Creates a database and opens the given file.

	 @param file the database file
	 @throws see database::open()
	*/
	database(gsl::not_null<gsl::czstring<>> file)
	{
		open(file);
	}
	database(const database& copy) = delete;
	/**
	 Move-Constructor.

	 @post `move` is closed

	 @param[in,out] move the other database
	*/
	database(database&& move) noexcept
	{
		_database      = move._database;
		move._database = nullptr;
	}

	/**
	 Destructor.

	 @post the database is closed

	 @see force-closing with database::close()
	*/
	virtual ~database()
	{
		close(true);
	}
	/**
	 Sets the database journaling mode.

	 @param mode the journal modus
	 @throws see database::execute()
	*/
	void set_journal_mode(journal_mode mode)
	{
		switch (mode) {
		case journal_mode::delete_: execute("PRAGMA journal_mode=DELETE;"); break;
		case journal_mode::truncate: execute("PRAGMA journal_mode=TRUNCATE;"); break;
		case journal_mode::persist: execute("PRAGMA journal_mode=PERSIST;"); break;
		case journal_mode::memory: execute("PRAGMA journal_mode=MEMORY;"); break;
		case journal_mode::wal: execute("PRAGMA journal_mode=WAL;"); break;
		case journal_mode::off: execute("PRAGMA journal_mode=OFF;"); break;
		}
	}
	/**
	 Enables or disables foreign key support.

	 @param enable whether to enable support or not
	 @throws see database::execute()
	*/
	void enable_foreign_keys(bool enable = true)
	{
		execute(enable ? "PRAGMA foreign_keys=ON" : "PRAGMA foreign_keys=OFF");
	}
	/**
	 Closes this database. Closing a closed database is a noop.

	 @param force (opt) if `true` the database is closed when every statement was finalized and/or backups finished
	 @throws exception::database_exception if `force == false` and if the database is busy
	*/
	void close(bool force = false)
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
	/**
	 Checks whether the database is closed. This function does not check if the actual database file was forcefully
	 closed.

	 @returns `true` if the database is closed, otherwise `false`
	*/
	bool closed() const noexcept
	{
		return _database == nullptr;
	}
	/**
	 Opens the given file with the specified parameters. If a database is already open, it will be closed.

	 @post the database is opened (`closed() == false`)

	 @param file the file name
	 @param flags (opt) the open flags, by default the file will be opened with `open_flag_readwrite |
	 open_flag_create`; see database::open_flag
	 @param vfs (opt) the virtual filesystem that should be used; if empty, the default vfs will be used
	 @throws exception::database_exception if the database could not be opened
	 @throws see database::close()
	*/
	void open(gsl::not_null<gsl::czstring<>> file, open_flag_type flags = open_flag_readwrite | open_flag_create,
	          gsl::czstring<> vfs = nullptr)
	{
		Expects(vfs == nullptr || vfs[0] != 0);

		close();

		auto error = sqlite3_open_v2(file, &_database, flags, vfs);

		if (error != SQLITE_OK) {
			_database = nullptr;

			YSQLITE_THROW(exception::database_exception, error, "failed to open");
		}

		Ensures(!closed());
	}
	/**
	 Registers a normal function which can then be used in SQL statements.

	 @tparam T the function; must inherit function::function
	 @tparam Args the arg types for instantiating `T` inplace
	 @param name the name of the function
	 @param args the arguments for `T`
	 @throws exception::database_exception if the function could not be registered
	 @throws see T()
	*/
	template<typename T, typename... Args>
	typename std::enable_if<std::is_base_of<function::function, T>::value>::type
	    register_function(gsl::not_null<gsl::czstring<>> name, Args... args)
	{
		Expects(!closed());

		auto instance = new T(std::forward<Args>(args)...);
		auto error    = sqlite3_create_function_v2(_database, name, instance->_argc, instance->_flags, instance,
                                                function::function::xfunc, nullptr, nullptr,
                                                [](void* instance) { delete static_cast<T*>(instance); });

		if (error != SQLITE_OK) {
			delete instance;

			YSQLITE_THROW(exception::database_exception, error, "could not create function");
		}
	}
	/**
	 Runs the SQL statement and ignores the result values.

	 @pre the database is opened

	 @param sql zero or more SQL statements
	 @returns the amount of returned result rows
	 @throws exception::database_exception if a database error occurred
	 @throws exception::sql_exception if the SQL statement is invalid
	*/
	std::size_t execute(gsl::not_null<gsl::czstring<>> sql)
	{
		Expects(!closed());

		char* message      = nullptr;
		std::size_t result = 0;
		auto error         = sqlite3_exec(
            _database, sql,
            [](void* result, int, char**, char**) {
                *static_cast<std::size_t*>(result) += 1;

                return SQLITE_OK;
            },
            &result, &message);

		if (message) {
			auto _ = gsl::finally([message] { sqlite3_free(message); });

			YSQLITE_THROW(exception::sql_exception, message);
		} else if (error != SQLITE_OK) {
			YSQLITE_THROW(exception::database_exception, error, "failed to execute sql");
		}

		return result;
	}
	/**
	 Creates a prepared statement. Prepared statement can have parameters descibed by any of the following:

	 - `?`
	 - `?NNN`
	 - `:VVV`
	 - `@VVV`
	 - `$VVV`

	 @pre the database is opened

	 @param sql the SQL statement
	 @returns the prepared statement
	 @throws exception::database_exception if the statement could not be created
	*/
	statement prepare_statement(gsl::not_null<gsl::czstring<>> sql)
	{
		Expects(!closed());

		sqlite3_stmt* stmt = nullptr;
		const char* tail   = nullptr;
		auto error         = sqlite3_prepare_v2(_database, sql, -1, &stmt, &tail);

		if (error != SQLITE_OK) {
			YSQLITE_THROW(exception::database_exception, error, "could not prepare statement");
		}

		return stmt;
	}
	/**
	 Begins a new transaction.

	 @returns the transaction object; use this to rollback or commit
	 @throws see database::execute()
	*/
	transaction begin_transaction()
	{
		execute("BEGIN TRANSACTION;");

		return { this };
	}
	/**
	 Returns the SQLite database file handle. This database will be marked as closed, but the handle will remain open.

	 @post the database is closed

	 @returns the handle
	*/
	gsl::owner<sqlite3*> release() noexcept
	{
		auto p = _database;

		_database = nullptr;

		return p;
	}
	/**
	 Returns the SQLite3 database handle.

	 @returns the database handle or `nullptr` if the database is closed
	*/
	sqlite3* handle() noexcept
	{
		return _database;
	}
	/**
	 Returns the SQLite3 database handle.

	 @returns the database handle or `nullptr` if the database is closed
	*/
	const sqlite3* handle() const noexcept
	{
		return _database;
	}

private:
	sqlite3* _database;
};

} // namespace ysqlite3