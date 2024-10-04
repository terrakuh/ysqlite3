#pragma once

#include "sqlite3.h"

#include <system_error>
#include <type_traits>

namespace ysqlite3 {

enum class SQLite3Condition
{
	success = 0,
	/** Generic error */
	generic = 1,
	/** Internal logic error in SQLite */
	internal = 2,
	/** Access permission denied */
	permission = 3,
	/** Callback routine requested an abort */
	abort = 4,
	/** The database file is locked */
	busy = 5,
	/** A table in the database is locked */
	locked = 6,
	/** A malloc() failed */
	memory = 7,
	/** Attempt to write a readonly database */
	read_only = 8,
	/** Operation terminated by sqlite3_interrupt()*/
	operation_interrupted = 9,
	/** Some kind of disk I/O error occurred */
	io = 10,
	/** The database disk image is malformed */
	corrput = 11,
	/** Unknown opcode in sqlite3_file_control() */
	not_found = 12,
	/** Insertion failed because database is full */
	database_full = 13,
	/** Unable to open the database file */
	bad_database = 14,
	/** Database lock protocol error */
	protocol = 15,
	/** Internal use only */
	empty = 16,
	/** The database schema changed */
	schema_changed = 17,
	/** String or BLOB exceeds size limit */
	value_too_big = 18,
	/** Abort due to constraint violation */
	sql_constraint = 19,
	/** Data type mismatch */
	type_mismatch = 20,
	/** Library used incorrectly */
	library_misuse = 21,
	/** Uses OS features not supported on host */
	no_lfs = 22,
	/** Authorization denied */
	auth_denied = 23,
	/** Not used */
	format = 24,
	/** 2nd parameter to sqlite3_bind out of range */
	out_of_range = 25,
	/** File opened that is not a database file */
	not_a_database = 26,
	/** Notifications from sqlite3_log() */
	notice = 27,
	/** Warnings from sqlite3_log() */
	warning = 28,
};

enum class SQLite3Error
{
	success = 0,
	/** Generic error */
	generic = 1,
	/** Internal logic error in SQLite */
	internal = 2,
	/** Access permission denied */
	permission = 3,
	/** Callback routine requested an abort */
	abort = 4,
	/** The database file is locked */
	busy = 5,
	/** A table in the database is locked */
	locked = 6,
	/** A malloc() failed */
	memory = 7,
	/** Attempt to write a readonly database */
	read_only = 8,
	/** Operation terminated by sqlite3_interrupt()*/
	operation_interrupted = 9,
	/** Some kind of disk I/O error occurred */
	io = 10,
	/** The database disk image is malformed */
	corrupt = 11,
	/** Unknown opcode in sqlite3_file_control() */
	not_found = 12,
	/** Insertion failed because database is full */
	database_full = 13,
	/** Unable to open the database file */
	bad_database = 14,
	/** Database lock protocol error */
	protocol = 15,
	/** Internal use only */
	empty = 16,
	/** The database schema changed */
	schema_changed = 17,
	/** String or BLOB exceeds size limit */
	value_too_big = 18,
	/** Abort due to constraint violation */
	sql_constraint = 19,
	/** Data type mismatch */
	type_mismatch = 20,
	/** Library used incorrectly */
	library_misuse = 21,
	/** Uses OS features not supported on host */
	no_lfs = 22,
	/** Authorization denied */
	auth_denied = 23,
	/** Not used */
	format = 24,
	/** 2nd parameter to sqlite3_bind out of range */
	out_of_range = 25,
	/** File opened that is not a database file */
	not_a_database = 26,
	/** Notifications from sqlite3_log() */
	notice = 27,
	/** Warnings from sqlite3_log() */
	warning = 28,

	missing_collseq    = (generic | (1 << 8)),
	retry              = (generic | (2 << 8)),
	snapshot           = (generic | (3 << 8)),
	read               = (io | (1 << 8)),
	short_read         = (io | (2 << 8)),
	write              = (io | (3 << 8)),
	fsync              = (io | (4 << 8)),
	dir_fsync          = (io | (5 << 8)),
	truncate           = (io | (6 << 8)),
	fstat              = (io | (7 << 8)),
	unlock             = (io | (8 << 8)),
	rdlock             = (io | (9 << 8)),
	delete_            = (io | (10 << 8)),
	blocked            = (io | (11 << 8)),
	nomem              = (io | (12 << 8)),
	access             = (io | (13 << 8)),
	checkreservedlock  = (io | (14 << 8)),
	lock               = (io | (15 << 8)),
	close              = (io | (16 << 8)),
	dir_close          = (io | (17 << 8)),
	shmopen            = (io | (18 << 8)),
	shmsize            = (io | (19 << 8)),
	shmlock            = (io | (20 << 8)),
	shmmap             = (io | (21 << 8)),
	seek               = (io | (22 << 8)),
	delete_noent       = (io | (23 << 8)),
	mmap               = (io | (24 << 8)),
	gettemppath        = (io | (25 << 8)),
	convpath           = (io | (26 << 8)),
	vnode              = (io | (27 << 8)),
	auth               = (io | (28 << 8)),
	begin_atomic       = (io | (29 << 8)),
	commit_atomic      = (io | (30 << 8)),
	rollback_atomic    = (io | (31 << 8)),
	data               = (io | (32 << 8)),
	corrupt_filesystem = (io | (33 << 8)),
	sharedcache        = (locked | (1 << 8)),
	vtab_locked        = (locked | (2 << 8)),
	recovery           = (busy | (1 << 8)),
	busy_snapshot      = (busy | (2 << 8)),
	timeout            = (busy | (3 << 8)),
	notempdir          = (bad_database | (1 << 8)),
	isdir              = (bad_database | (2 << 8)),
	fullpath           = (bad_database | (3 << 8)),
	bad_convpath       = (bad_database | (4 << 8)),
	dirtywal           = (bad_database | (5 << 8)) /* Not Used */,
	bad_symlink        = (bad_database | (6 << 8)),
	corrupt_vtab       = (corrupt | (1 << 8)),
	sequence           = (corrupt | (2 << 8)),
	index              = (corrupt | (3 << 8)),
	recovery_denied    = (read_only | (1 << 8)),
	cantlock           = (read_only | (2 << 8)),
	rollback           = (read_only | (3 << 8)),
	dbmoved            = (read_only | (4 << 8)),
	cantinit           = (read_only | (5 << 8)),
	directory          = (read_only | (6 << 8)),
	rollback_aborted   = (abort | (2 << 8)),
	check              = (sql_constraint | (1 << 8)),
	commithook         = (sql_constraint | (2 << 8)),
	foreignkey         = (sql_constraint | (3 << 8)),
	function           = (sql_constraint | (4 << 8)),
	notnull            = (sql_constraint | (5 << 8)),
	primarykey         = (sql_constraint | (6 << 8)),
	trigger            = (sql_constraint | (7 << 8)),
	unique             = (sql_constraint | (8 << 8)),
	vtab               = (sql_constraint | (9 << 8)),
	rowid              = (sql_constraint | (10 << 8)),
	pinned             = (sql_constraint | (11 << 8)),
	recover_wal        = (notice | (1 << 8)),
	recover_rollback   = (notice | (2 << 8)),
	autoindex          = (warning | (1 << 8)),
	user               = (auth_denied | (1 << 8)),
	load_permanently   = (success | (1 << 8)),
	symlink            = (success | (2 << 8)),
};

[[nodiscard]] inline const std::error_category& sqlite3_category() noexcept
{
	static class : public std::error_category
	{
	public:
		const char* name() const noexcept override
		{
			return "sqlite3";
		}
		std::string message(int ec) const override
		{
			if (const auto str = sqlite3_errstr(ec)) {
				return str;
			}
			return "(unknown SQLite3 error)";
		}
	} category;
	return category;
}

[[nodiscard]] inline std::error_code make_error_code(SQLite3Error ec) noexcept
{
	return { static_cast<int>(ec), sqlite3_category() };
}

enum class Error
{
	success,

	// bad input
	bad_vfs_name,
	null_function,

	database_is_closed,
	statement_is_closed,
	unknown_parameter,
	parameter_out_of_range,
	bad_index_name,
	bad_arguments,
	bad_result,
	vfs_already_registered,
	out_of_bounds,
	numeric_narrowing,
};

enum class Condition
{
	success,
	bad_input,
};

[[nodiscard]] inline std::error_code make_error_code(Error ec) noexcept
{
	static class : public std::error_category
	{
	public:
		const char* name() const noexcept override
		{
			return "ysqlite3";
		}
		std::string message(int ec) const override
		{
			switch (static_cast<Error>(ec)) {
			case Error::success: return "success";

			case Error::bad_vfs_name: return "bad VFS name";
			case Error::null_function: return "null function";

			case Error::database_is_closed: return "database is closed";
			case Error::statement_is_closed: return "statement is closed";
			case Error::unknown_parameter: return "unknown parameter";
			case Error::parameter_out_of_range: return "parameter is out of range";
			case Error::bad_index_name: return "bad index name";
			case Error::bad_arguments: return "bad argument";
			case Error::bad_result: return "bad result";
			case Error::vfs_already_registered: return "VFS is already registered";
			case Error::out_of_bounds: return "out of bounds";
			case Error::numeric_narrowing: return "numeric narrowing";
			default: return "(unknown error code)";
			}
		}
	} category;
	return { static_cast<int>(ec), category };
}

} // namespace ysqlite3

namespace std {

template<>
struct is_error_condition_enum<ysqlite3::SQLite3Condition> : true_type
{};

template<>
struct is_error_code_enum<ysqlite3::SQLite3Error> : true_type
{};

template<>
struct is_error_code_enum<ysqlite3::Error> : true_type
{};

template<>
struct is_error_condition_enum<ysqlite3::Condition> : true_type
{};

} // namespace std
