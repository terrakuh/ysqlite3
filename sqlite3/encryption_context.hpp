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
	typedef std::function<int(void*, int, int64_t)> read_callback_signature;

	void set_read_callback(read_callback_signature && _callback) noexcept;
	void set_read_callback(const read_callback_signature & _callback) noexcept;
	void set_sector_size(size_t _size) noexcept;
	int sync() noexcept;
	virtual bool does_something() const noexcept = 0;
	int64_t last_encryption_offset() const noexcept;
	virtual void encrypt(int64_t _id, const void * _buffer, size_t _size, void * _output) = 0;
	virtual void decrypt(int64_t _id, const void * _buffer, size_t _size, void * _output) = 0;
	virtual std::vector<std::tuple<const void*, size_t, int64_t>> modified_overhead_sectors() = 0;

protected:
	size_t _sector_size;
	int64_t _last_encryption_offset;

	bool can_read() const noexcept;
	int read(void * _output, int _size, int64_t _offset) noexcept;
	virtual size_t header_size() const noexcept = 0;


private:
	read_callback_signature _read_callback;
};

}