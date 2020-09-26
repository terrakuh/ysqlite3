#ifndef YSQLITE3_DATABASE_HPP_
#define YSQLITE3_DATABASE_HPP_

#include "error.hpp"
#include "function/function.hpp"
#include "sqlite3.h"
#include "statement.hpp"
#include "transaction.hpp"

#include <cstddef>
#include <memory>
#include <type_traits>

namespace ysqlite3 {

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

enum class text_encoding
{
	utf8    = SQLITE_UTF8,
	utf16be = SQLITE_UTF16BE,
	utf16le = SQLITE_UTF16LE
};

/**
 * The database class. This class manages a single database connection.
 */
class database
{
public:
	/**
	 * Creates a database without a connection.
	 */
	database() = default;
	/**
	 * Creates a database and opens the given file.
	 *
	 * @exception see database::open()
	 * @param file the database file
	 */
	database(const char* file);
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
	 * @exception see database::execute()
	 * @param mode the journal modus
	 */
	void set_journal_mode(journal_mode mode);
	/**
	 * Enables or disables foreign key support.
	 *
	 * @exception see database::execute()
	 * @param enable whether to enable support or not
	 */
	void enable_foreign_keys(bool enable = true);
	/**
	 * Closes this database. Closing a closed database is a noop.
	 *
	 * @exception std::system_error if `force == false` and if the database is busy
	 * @param force if `true` the database is closed when every statement was finalized and/or backups
	 * finished
	 */
	void close(bool force = false);
	bool is_open() const noexcept;
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
	void open(const char* file, open_flag_type flags = open_flag_readwrite | open_flag_create,
	          const char* vfs = nullptr);
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
	template<typename Type, typename... Args>
	typename std::enable_if<std::is_base_of<function::function, Type>::value>::type
	    register_function(const char* name, Args... args)
	{
		if (!is_open()) {
			throw std::system_error{ errc::database_is_closed };
		}

		std::unique_ptr<Type> instance{ new Type{ std::forward<Args>(args)... } };
		const auto ec = sqlite3_create_function_v2(
		    _database, name, instance->_argc, instance->_flags, instance.get(), function::function::xfunc,
		    nullptr, nullptr, [](void* instance) { delete static_cast<Type*>(instance); });

		if (ec) {
			throw std::system_error{ static_cast<sqlite3_errc>(ec) };
		}

		instance.release();
	}
	inline void register_function(const char* name, void (*function)(sqlite3_context*, int, sqlite3_value**),
	                              function::text_encoding encoding, bool deterministic = true,
	                              bool direct_only = false)
	{
		if (!is_open()) {
			throw std::system_error{ errc::database_is_closed };
		}

		const auto ec =
		    sqlite3_create_function_v2(_database, name, -1,
		                               static_cast<int>(encoding) | SQLITE_DETERMINISTIC * deterministic |
		                                   SQLITE_DIRECTONLY * direct_only,
		                               nullptr, function, nullptr, nullptr, nullptr);

		if (ec) {
			throw std::system_error{ static_cast<sqlite3_errc>(ec) };
		}
	}
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
	/**
	 * Returns the rowid of the last insert statement of this connection or 0 if no insert was made.
	 *
	 * @return the last rowid
	 */
	sqlite3_int64 last_insert_rowid() const noexcept;
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
	 * @exception std::system_error if the statement could not be created
	 * @param sql the SQL statement
	 * @return the prepared statement
	 */
	statement prepare_statement(const char* sql);
	/**
	 * Returns the SQLite database file handle. This database will be marked as closed, but the handle will
	 * remain open.
	 *
	 * @post the database is closed
	 *
	 * @return the handle or `nullptr`
	 */
	sqlite3* release_handle() noexcept;
	/**
	 * Returns the SQLite3 database handle.
	 *
	 * @return the database handle or `nullptr` if the database is closed
	 */
	sqlite3* handle() noexcept;
	/**
	 * Returns the SQLite3 database handle.
	 *
	 * @return the database handle or `nullptr` if the database is closed
	 */
	const sqlite3* handle() const noexcept;
	database& operator=(database&& move) noexcept;

private:
	friend transaction;

	/** the underlying sqlite3 database connection */
	sqlite3* _database = nullptr;

	template<typename Functor>
	static void _functor_forwarder(sqlite3_context* context, int argc, sqlite3_value** argv)
	{
		(*static_cast<Functor*>(sqlite3_user_data(context)))(context, argc, argv);
	}
};

} // namespace ysqlite3

#endif
