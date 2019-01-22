#pragma once

#include <stdexcept>


namespace ysqlite3
{

class database_error : public std::runtime_error
{
public:
	using std::runtime_error::runtime_error;
};

class programming_error : public database_error
{
public:
	using database_error::database_error;
};

class key_error : public database_error
{
public:
	using database_error::database_error;
};

}