#ifndef YSQLITE3_VFS_PAGE_TRANSFORMING_FILE_HPP_
#define YSQLITE3_VFS_PAGE_TRANSFORMING_FILE_HPP_

#include "../error.hpp"
#include "file.hpp"

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>
#include <vector>

namespace ysqlite3 {
namespace vfs {

inline const char* name(ysqlite3::vfs::file_format format) noexcept
{
	using namespace ysqlite3::vfs;
	switch (format) {
	case file_format::main_db: return "main db";
	case file_format::main_journal: return "main journal";
	default: return "unkown";
	}
}

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
		printf("read from %s: %zi from %lli\n", name(this->format), buffer.size(), offset);
#endif

		if (_is_page(buffer.size(), offset)) {
			Parent::read(buffer, offset);
			decode_page(buffer);
		} else {
#if PRINT_DEBUG
			puts("forwarding read");
#endif
			Parent::read(buffer, offset);
		}

		// check reserve size
		if (this->format == file_format::main_db && offset <= 20 && offset + buffer.size() >= 21) {
			if (!check_reserve_size(buffer.begin()[20 - offset])) {
				throw std::system_error{ sqlite3_errc::generic, "bad reserve size" };
			}
		}
	}
	void write(span<const std::uint8_t*> buffer, sqlite3_int64 offset) override
	{
#if PRINT_DEBUG
		printf("write to %s: %zi from %lli\n", name(this->format), buffer.size(), offset);
#endif

		// check reserve size
		if (this->format == file_format::main_db && offset <= 20 && offset + buffer.size() >= 21) {
			if (!check_reserve_size(buffer.begin()[20 - offset])) {
				throw std::system_error{ sqlite3_errc::generic, "bad reserve size" };
			}
		}

		if (_is_page(buffer.size(), offset)) {
			_tmp_buffer.resize(buffer.size());
			std::memcpy(_tmp_buffer.data(), buffer.begin(), _tmp_buffer.size());
			encode_page({ _tmp_buffer.data(), _tmp_buffer.size() });
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

	bool _is_page(std::size_t size, sqlite3_int64 offset) const noexcept
	{
		return (this->format == file_format::main_db /* ||
		        (this->format == file_format::main_journal && offset) */) &&
		       size >= 512;
	}
};

} // namespace vfs
} // namespace ysqlite3

#endif
