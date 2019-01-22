#include "address_transporter.hpp"

#include <sstream>
#include <cstdint>


namespace ysqlite3
{

const std::regex address_transporter::_pattern(R"((.*?)-([0-9a-fA-F]+)-(.*))");


void address_transporter::encode_address(std::string & _str, void * _adderss)
{
	std::stringstream _stream;

	_stream << _str << '-' << std::hex << reinterpret_cast<intptr_t>(_adderss) << '-';
	
	_str = _stream.str();
}

void * address_transporter::decode_address(std::string & _str)
{
	std::smatch _match;
	uintptr_t _address = 0;

	if (std::regex_search(_str, _match, _pattern)) {
		std::stringstream _stream(_match[2].str());

		_stream >> std::hex >> _address;

		_str = _match[1].str() + _match[3].str();
	}

	return reinterpret_cast<void*>(_address);
}

}