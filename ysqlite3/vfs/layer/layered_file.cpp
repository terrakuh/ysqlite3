#include "layered_file.hpp"

using namespace ysqlite3::vfs::layer;

inline bool is_page(ysqlite3::vfs::file_format format, std::size_t size, sqlite3_int64 offset) noexcept
{
	using namespace ysqlite3::vfs;
	return format == file_format::main_db && size >= 512;
}

inline std::pair<std::size_t, sqlite3_int64> transform_bounds(ysqlite3::vfs::file_format format,
                                                              std::size_t size, sqlite3_int64 offset) noexcept
{
	using namespace ysqlite3::vfs;

	if (format == file_format::main_db) {
		size = 4096;
		offset -= offset % 4096;
	}

	return { size, offset };
}

inline const char* name(ysqlite3::vfs::file_format format)
{
	using namespace ysqlite3::vfs;
	switch (format) {
	case file_format::main_db: return "main db";
	case file_format::main_journal: return "main journal";
	default: return "unkown";
	}
}

layered_file::layered_file(file_format format, std::unique_ptr<file> parent, layers_type layers) noexcept
    : file{ format }, _parent{ std::move(parent) }, _layers{ std::move(layers) }
{
	for (const auto& layer : _layers) {
		_data_size += layer->data_size();
	}
}

void layered_file::close()
{
	_parent->close();
}

void layered_file::read(span<std::uint8_t*> buffer, sqlite3_int64 offset)
{
	printf("read from %s: %zi from %lli\n", name(format), buffer.size(), offset);

	if (format == file_format::main_db || (format == file_format::main_journal && offset != 0)) {
		if (buffer.size() >= 512) {
			_parent->read(buffer, offset);
			_decode_page(buffer);
		} else {
			std::vector<std::uint8_t> tmp;
			tmp.resize(4096);
			_parent->read({ tmp.data(), tmp.size() }, offset - offset % 4096);
			_decode_page({ tmp.data(), tmp.size() });
			std::memcpy(buffer.begin(), tmp.data() + offset % 4096, buffer.size());
		}
	} else {
		puts("forwarding read");
		_parent->read(buffer, offset);
	}
}

void layered_file::write(span<const std::uint8_t*> buffer, sqlite3_int64 offset)
{
	printf("write to %s: %zi from %lli\n", name(format), buffer.size(), offset);

	if ((format == file_format::main_db || (format == file_format::main_journal && offset != 0)) &&
	    buffer.size() >= 512) {
		std::vector<std::uint8_t> tmp;
		tmp.resize(buffer.size());
		std::memcpy(tmp.data(), buffer.begin(), tmp.size());
		_encode_page({ tmp.data(), tmp.size() });
		_parent->write({ tmp.data(), tmp.size() }, offset);
	} else {
		puts("forwarding write");
		_parent->write(buffer, offset);
	}
}

void layered_file::truncate(sqlite3_int64 size)
{
	throw std::system_error{ sqlite3_errc::io };
	_parent->truncate(size);
}

void layered_file::sync(sync_flag flag)
{
	_parent->sync(flag);
}

sqlite3_int64 layered_file::file_size() const
{
	return _parent->file_size();
}

void layered_file::lock(lock_flag flag)
{
	_parent->lock(flag);
}

void layered_file::unlock(lock_flag flag)
{
	_parent->unlock(flag);
}
bool layered_file::has_reserved_lock() const
{
	return _parent->has_reserved_lock();
}

void layered_file::file_control(file_cntl operation, void* arg)
{
	// pass pragma to all layers
	if (operation == file_cntl::file_cntl_pragma) {
		for (auto& layer : _layers) {
			if (layer->pragma(static_cast<char**>(arg)[1], static_cast<char**>(arg)[2])) {
				return;
			}
		}
	}

	_parent->file_control(operation, arg);
}

int layered_file::sector_size() const noexcept
{
	return _parent->sector_size();
}

int layered_file::device_characteristics() const noexcept
{
	return _parent->device_characteristics();
}

void layered_file::_encode_page(span<std::uint8_t*> page) const
{
	auto data_offset = _data_size;
	for (const auto& layer : _layers) {
		const auto data_size = layer->data_size();
		layer->encode(page.subspan(0, page.size() - data_offset),
		              page.subspan(page.size() - data_offset, data_size));
		data_offset -= data_size;
	}
}

void layered_file::_decode_page(span<std::uint8_t*> page) const
{
	for (auto i = _layers.rbegin(); i != _layers.rend(); ++i) {
		const auto& layer    = *i;
		const auto data_size = layer->data_size();
		layer->decode(page.subspan(0, page.size() - data_size),
		              page.subspan(page.size() - data_size).cast<const std::uint8_t*>());
		page = page.subspan(0, page.size() - data_size);
	}
}
