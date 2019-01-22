#pragma once

#include <string>
#include <regex>


namespace ysqlite3
{

class address_transporter
{
public:
	static void encode_address(std::string & _str, void * _adderss);
	static void * decode_address(std::string & _str);

private:
	const static std::regex _pattern;
};

}