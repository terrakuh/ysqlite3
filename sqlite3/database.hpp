#pragma once

#include <memory>
#include <stdexcept>
#include <string>


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

class database_error : public std::runtime_error
{
public:
	database_error(const std::string & _message) : std::runtime_error(_message)
	{
	}
	database_error(const char * _message) : std::runtime_error(_message)
	{
	}
};

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
	database(const char * _filename, int _mode = SO_READWRITE | SO_CREATE);
	/**
	 * Destructor.
	 *
	 * @since 1.0.0.1
	 * @date 29-Sep-18
	*/
	virtual ~database() noexcept = default;
	void execute(const char * _sql);

protected:
	virtual bool open(const char * _filename, int _mode);

private:
	struct impl;

	std::unique_ptr<impl> _impl;
};

}