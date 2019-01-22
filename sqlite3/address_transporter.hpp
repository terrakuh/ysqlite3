#pragma once

#include <string>
#include <regex>

#include "config.hpp"


namespace ysqlite3
{

class address_transporter
{
public:
	SQLITE3_API static void encode_address(std::string & _str, void * _adderss);
	SQLITE3_API static void * decode_address(std::string & _str);

private:
	const static std::regex _pattern;
};

}