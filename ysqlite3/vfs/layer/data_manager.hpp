#ifndef YSQLITE3_VFS_LAYER_DATA_MANAGER_HPP_
#define YSQLITE3_VFS_LAYER_DATA_MANAGER_HPP_

#include "../../span.hpp"
#include "../../sqlite3.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace ysqlite3 {
namespace vfs {
namespace layer {

class data_manager
{
public:
	data_manager(std::uint8_t data_size, std::uint32_t page_size = 4096)
	{
		_data_size = data_size;
		_page_size = page_size;
		_data.resize(4096);
	}
	sqlite3_int64 translate_offset(sqlite3_int64 offset) const noexcept
	{
		return offset + 4096;
	}
	sqlite3_int64 translate_file_size(sqlite3_int64 file_size) const noexcept
	{
		return file_size - 4096;
	}
	template<typename Reader>
	span<const std::uint8_t*> load(Reader&& reader, sqlite3_int64 offset)
	{
		const auto index       = offset / _page_size;
		const auto data_offset = 0;
		reader(span<std::uint8_t*>{ _data.data(), _data.size() }, data_offset);
		return { _data.data() + index * _data_size, _data_size };
	}
	template<typename Reader, typename Writer>
	void save(Reader&& reader, Writer&& writer, span<const std::uint8_t*> data, sqlite3_int64 offset)
	{
		//assert(data.size() == _data_size);
		const auto index       = offset / _page_size;
		const auto data_offset = 0;
		if (std::memcmp(_data.data() + index * _data_size, data.begin(), _data_size)) {
			std::memcpy(_data.data() + index * _data_size, data.begin(), _data_size);
			writer(span<const std::uint8_t*>{ _data.data(), _data.size() }, data_offset);
		}
	}
	std::uint8_t data_size() const noexcept
	{
		return _data_size;
	}

private:
	std::vector<std::uint8_t> _data;
	std::uint8_t _data_size;
	std::uint32_t _page_size;
};

} // namespace layer
} // namespace vfs
} // namespace ysqlite3

#endif
