#pragma once

#include <string>
#include <cstdio>
#include <cstdint>


class base64_error : public std::runtime_error
{
public:
	std::runtime_error::runtime_error;
};

class base64
{
public:
	constexpr static char pad = '=';

	enum class ALPHABET
	{
		AUTO,
		NORMAL,
		URL_FILENAME_SAFE
	};

	static void decode_inplace(std::string & _str, ALPHABET _alphabet = ALPHABET::AUTO)
	{
		_str.resize(decode_inplace(&_str[0], _str.length(), _alphabet));
	}
	static size_t decode_inplace(char * _str, size_t _length, ALPHABET _alphapet = ALPHABET::AUTO)
	{
		if (!_length) {
			return 0;
		} else if (_length % 4) {
			throw base64_error("invalid base64 input.");
		}

		return _length / 4 * 3 - do_decode(_str, _length, _str, _alphapet);
	}
	static std::string encode(const std::string & _str, ALPHABET _alphabet = ALPHABET::NORMAL)
	{
		return encode(_str.c_str(), _str.length(), _alphabet);
	}
	static std::string encode(const char * _str, size_t _length, ALPHABET _alphabet = ALPHABET::NORMAL)
	{
		if (!_length) {
			return "";
		}

		std::string _result;
		const char * _alpha = nullptr;
		const auto _end = _str + _length;

		// Set alphabet
		switch (_alphabet) {
		case ALPHABET::AUTO:
		case ALPHABET::NORMAL:
			_alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

			break;
		case ALPHABET::URL_FILENAME_SAFE:
			_alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

			break;
		}

		_result.resize((_length / 3 + (_length % 3 ? 1 : 0)) * 4);

		auto _out = &_result[0];

		// Encode 3 bytes
		while (_str + 3 <= _end) {
			encode<3>(_alpha, _str, _out);
		}

		// Encode last bit
		if (_end - _str == 1) {
			encode<1>(_alpha, _str, _out);
		} else if (_end - _str == 2) {
			encode<2>(_alpha, _str, _out);
		}

		return _result;
	}
	static std::string decode(const std::string & _str, ALPHABET _alphabet = ALPHABET::AUTO)
	{
		return decode(_str.c_str(), _str.length(), _alphabet);
	}
	static std::string decode(const char * _str, size_t _length, ALPHABET _alphabet = ALPHABET::AUTO)
	{
		if (!_length) {
			return "";
		} else if (_length % 4) {
			throw base64_error("invalid base64 input.");
		}

		std::string _result;
		const auto _end = _str + _length;

		_result.resize(_length / 4 * 3);
		_result.resize(_result.length() - do_decode(_str, _length, &_result[0], _alphabet));

		return _result;
	}

private:
	template<int Count>
	static void encode(const char * _alphabet, const char *& _input, char *& _output)
	{
		if (Count == 3) {
			_output[0] = _alphabet[_input[0] >> 2 & 0x3f];
			_output[1] = _alphabet[(_input[0] & 0x03) << 4 | _input[1] >> 4 & 0x0f];
			_output[2] = _alphabet[(_input[1] & 0x0f) << 2 | _input[2] >> 6 & 0x03];
			_output[3] = _alphabet[_input[2] & 0x3f];
		} else if (Count == 2) {
			_output[0] = _alphabet[_input[0] >> 2 & 0x3f];
			_output[1] = _alphabet[(_input[0] & 0x03) << 4 | _input[1] >> 4 & 0x0f];
			_output[2] = _alphabet[(_input[1] & 0x0f) << 2];
			_output[3] = pad;
		} else if (Count == 1) {
			_output[0] = _alphabet[_input[0] >> 2 & 0x3f];
			_output[1] = _alphabet[(_input[0] & 0x03) << 4];
			_output[2] = pad;
			_output[3] = pad;
		}

		_input += Count;
		_output += 4;
	}
	template<int Count>
	static void decode(ALPHABET & _alphabet, const char *& _input, char *& _output)
	{
		char _tmp;

		_output[0] = base64_value(_alphabet, _input[0]) << 2;

		_tmp = base64_value(_alphabet, _input[1]);

		_output[0] |= _tmp >> 4;

		if (Count < 2) {
			_output[1] = (_tmp & 0x0f) << 4;

			_tmp = base64_value(_alphabet, _input[2]);

			_output[1] |= _tmp >> 2;

			if (Count < 1) {
				_output[2] = (_tmp & 0x03) << 6;
				_output[2] |= base64_value(_alphabet, _input[3]);
			}
		}

		_input += 4;
		_output += 3 - Count;
	}
	static char base64_value(ALPHABET & _alphabet, char _char)
	{
		if (_char >= 'A' && _char <= 'Z') {
			return _char - 'A';
		} else if (_char >= 'a' && _char <= 'z') {
			return _char - 'a' + 26;
		} else if (_char >= '0' && _char <= '9') {
			return _char - '0' + 52;
		}

		// Comes down to alphabet
		if (_alphabet == ALPHABET::NORMAL) {
			if (_char == '+') {
				return 62;
			} else if (_char == '/') {
				return 63;
			}
		} else if (_alphabet == ALPHABET::URL_FILENAME_SAFE) {
			if (_char == '-') {
				return 62;
			} else if (_char == '_') {
				return 63;
			}
		} else {
			if (_char == '+') {
				_alphabet = ALPHABET::NORMAL;

				return 62;
			} else if (_char == '/') {
				_alphabet = ALPHABET::NORMAL;

				return 63;
			} else if (_char == '-') {
				_alphabet = ALPHABET::URL_FILENAME_SAFE;

				return 62;
			} else if (_char == '_') {
				_alphabet = ALPHABET::URL_FILENAME_SAFE;

				return 63;
			}
		}

		throw base64_error("invalid base64 character.");
	}
	static size_t do_decode(const char * _input, size_t _length, char * _output, ALPHABET _alphabet)
	{
		const auto _end = _input + _length - 4;

		// Decode 4 bytes
		while (_input + 4 <= _end) {
			decode<0>(_alphabet, _input, _output);
		}

		// Decode last 4 bytes
		if (_input[3] == pad) {
			if (_input[2] == pad) {
				decode<2>(_alphabet, _input, _output);

				return 2;
			}

			decode<1>(_alphabet, _input, _output);

			return 1;
		}

		decode<0>(_alphabet, _input, _output);

		return 0;
	}
};