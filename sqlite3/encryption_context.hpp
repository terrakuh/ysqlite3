#pragma once

#include <cstdint>
#include <vector>
#include <tuple>
#include <functional>


namespace sqlite3
{

class encryption_context
{
public:
	virtual void encrypt(int64_t _id, const void * _buffer, size_t _size, void * _output) = 0;
	virtual void decrypt(int64_t _id, const void * _buffer, size_t _size, void * _output, std::function<int(void*, int, int64_t)> _read_callback) = 0;
	virtual std::vector<std::tuple<const void*, size_t, int64_t>> modified_overhead_sectors() = 0;
};

}