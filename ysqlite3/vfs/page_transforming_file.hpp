#ifndef YSQLITE3_VFS_PAGE_TRANSFORMING_FILE_HPP_
#define YSQLITE3_VFS_PAGE_TRANSFORMING_FILE_HPP_

#include "../error.hpp"
#include "file.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>
#include <vector>

namespace ysqlite3 {
namespace vfs {

template<typename Parent>
class page_transforming_file : public Parent
{
public:
	static_assert(std::is_base_of<file, Parent>::value, "parent must derive file");

	using page_transformer = page_transforming_file<Parent>;
	using Parent::Parent;

	void read(span<std::uint8_t*> buffer, sqlite3_int64 offset) override
	{
#if PRINT_DEBUG
		printf("read from %s: %zi from %lli\n", name_of(this->format), buffer.size(), offset);
#endif

		if (_is_page(buffer.size(), offset)) {
			Parent::read(buffer, offset);

			// do not decode header
			if (this->format == file_format::main_db && !offset) {
				decode_page(buffer.subspan(100));
			} else {
				decode_page(buffer);
			}

			_parse_parameters(buffer.cast<const std::uint8_t*>(), offset);
		} else {
#if PRINT_DEBUG
			puts("forwarding read");
#endif
			Parent::read(buffer, offset);
		}
	}
	void write(span<const std::uint8_t*> buffer, sqlite3_int64 offset) override
	{
#if PRINT_DEBUG
		printf("write to %s: %zi from %lli\n", name_of(this->format), buffer.size(), offset);
#endif

		if (_is_page(buffer.size(), offset)) {
			_parse_parameters(buffer, offset);
			_tmp_buffer.resize(buffer.size());
			std::memcpy(_tmp_buffer.data(), buffer.begin(), _tmp_buffer.size());

			// do not encode header
			if (this->format == file_format::main_db && !offset) {
				encode_page({ _tmp_buffer.data() + 100, _tmp_buffer.size() - 100 });
			} else {
				encode_page({ _tmp_buffer.data(), _tmp_buffer.size() });
			}

			Parent::write({ _tmp_buffer.data(), _tmp_buffer.size() }, offset);
		} else {
#if PRINT_DEBUG
			puts("forwarding write");
#endif
			Parent::write(buffer, offset);
		}
	}

protected:
	virtual void encode_page(span<std::uint8_t*> page) = 0;
	virtual void decode_page(span<std::uint8_t*> page) = 0;
	virtual bool check_reserve_size(std::uint8_t size) const noexcept
	{
		return true;
	}

private:
	std::vector<std::uint8_t> _tmp_buffer;
	std::uint32_t _page_size = 0;

	bool _is_page(std::size_t size, sqlite3_int64 offset) const noexcept
	{
		return (this->format == file_format::main_db /* ||
		        (this->format == file_format::main_journal && offset) */) &&
		       size >= 512;
	}
	void _parse_parameters(span<const std::uint8_t*> buffer, sqlite3_int64 offset)
	{
		if (this->format == file_format::main_db) {
			// read page size
			if (offset <= 16 && offset + buffer.size() >= 18) {
				_page_size = buffer.begin()[16 - offset] << 8 | buffer.begin()[17 - offset];
				if (_page_size == 1) {
					_page_size = 65536;
				}
			}

			// check reserve size
			if (offset <= 20 && offset + buffer.size() >= 21 &&
			    !check_reserve_size(buffer.begin()[20 - offset])) {
				throw std::system_error{ sqlite3_errc::generic, "bad reserve size" };
			}
		}
	}
};

} // namespace vfs
} // namespace ysqlite3

#endif
