#pragma once

#include <stdexcept>


namespace sqlite3
{

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

class programming_error : public database_error
{
public:
	programming_error(const std::string & _message) : database_error(_message)
	{
	}
	programming_error(const char * _message) : database_error(_message)
	{
	}
};

}