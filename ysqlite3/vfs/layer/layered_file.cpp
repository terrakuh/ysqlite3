#include "layered_file.hpp"

using namespace ysqlite3::vfs::layer;

inline bool is_page(ysqlite3::vfs::file_format format, std::size_t size, sqlite3_int64 offset) noexcept
{
	return false;
}

inline std::pair<std::size_t, sqlite3_int64> transform_bounds(ysqlite3::vfs::file_format format,
                                                              std::size_t size, sqlite3_int64 offset) noexcept
{
	return { size, offset };
}

layered_file::layered_file(file_format format, std::unique_ptr<file> parent, layers_type layers) noexcept
    : file{ format }, _parent{ std::move(parent) }, _layers{ std::move(layers) }, _data_manager{ _data_size(
	                                                                                  _layers) }
{}

void layered_file::close()
{
	_parent->close();
}

void layered_file::read(span<std::uint8_t*> buffer, sqlite3_int64 offset)
{
	printf("read offset: %lli, size: %zi\n", offset, buffer.size());

	if (is_page(format, buffer.size(), offset)) {
		// transfrom bounds
		const auto bounds = transform_bounds(format, buffer.size(), offset);
		auto page         = buffer;
		// read

		// transform page
		for (auto i = _layers.rbegin(); i != _layers.rend(); ++i) {
			const auto& layer    = *i;
			const auto data_size = layer->data_size();
			layer->decode(page.subspan(0, page.size() - data_size),
			              page.subspan(page.size() - data_size).cast<const std::uint8_t*>());
			page = page.subspan(0, page.size() - data_size);
		}

		// copy only requested parts
	} else {
		_parent->read(buffer, offset);
	}

	// auto data = _data_manager.load(
	//     [&](span<std::uint8_t*> buffer, sqlite3_int64 offset) { _parent->read(buffer, offset); }, offset);

	// // read from parent
	// std::vector<std::uint8_t> tmp;
	// tmp.resize(4096);
	// _parent->read({ tmp.data(), tmp.size() }, _data_manager.translate_offset(offset / 4096 * 4096));

	// // decode all layers
	// for (auto i = _layers.rbegin(); i != _layers.rend(); ++i) {
	// 	const auto size = (*i)->data_size();
	// 	(*i)->decode({ tmp.data(), tmp.size() }, data.subspan(0, size), offset);
	// 	data = data.subspan(size);
	// }

	// std::memcpy(buffer.begin(), tmp.data() + offset % 4096, buffer.size());

	// printf("read offset: %lli, size: %zi\n", offset, buffer.size());
	// // read from parent
	// _parent->read(buffer, offset);

	// const auto content = buffer.subspan(0, buffer.size() - _data_size);
	// auto data          = buffer.subspan(content.size());

	// // remove all layers
	// for (auto i = _layers.rbegin(); i != _layers.rend(); ++i) {
	// 	const auto size = (*i)->data_size();
	// 	(*i)->decode(content, data.subspan(0, size), offset);
	// 	data = data.subspan(size);
	// }

	// if (offset <= 20 && offset + buffer.size() >= 21) {
	// 	buffer.begin()[20 - offset] = _data_size;
	// }
	// printf("readed offset: %lli, size: %zi\n", offset, buffer.size());
}

void layered_file::write(span<const std::uint8_t*> buffer, sqlite3_int64 offset)
{
	printf("write offset: %lli, size: %zi\n", offset, buffer.size());

	if (is_page(format, buffer.size(), offset)) {
		// transfrom bounds
		const auto bounds = transform_bounds(format, buffer.size(), offset);
		span<std::uint8_t*> page;
		// read

		// transform page
		for (const auto& layer : _layers) {
			const auto data_size = layer->data_size();
			layer->encode(page.subspan(0, page.size() - _data_size),
			              page.subspan(page.size() - _data_size, data_size));
		}

		// copy only requested parts
	} else {
		_parent->write(buffer, offset);
	}

	// std::vector<std::uint8_t> tmp;
	// tmp.resize(buffer.size() + _data_manager.data_size());
	// std::memcpy(tmp.data(), buffer.begin(), buffer.size());
	// span<std::uint8_t*> content{ tmp.data(), buffer.size() };
	// span<std::uint8_t*> data{ content.end(), _data_manager.data_size() };

	// // encode all layers
	// for (auto& layer : _layers) {
	// 	const auto size = layer->data_size();
	// 	layer->encode(content, data.subspan(data.size() - size), offset);
	// 	data = data.subspan(0, data.size() - size);
	// }

	// // forward to parent
	// _parent->write(content.cast<const std::uint8_t*>(), _data_manager.translate_offset(offset));
	// _data_manager.save(
	//     [&](span<std::uint8_t*> buffer, sqlite3_int64 offset) { _parent->read(buffer, offset); },
	//     [&](span<const std::uint8_t*> buffer, sqlite3_int64 offset) { _parent->write(buffer, offset); },
	//     { content.end(), _data_manager.data_size() }, offset);

	// printf("write offset: %lli, size: %zi\n", offset, buffer.size());
	// std::vector<std::uint8_t> tmp;
	// tmp.resize(buffer.size());
	// std::memcpy(tmp.data(), buffer.begin(), buffer.size());
	// span<std::uint8_t*> content{ tmp.data(), buffer.size() - _data_size };
	// span<std::uint8_t*> data{ tmp.data() + content.size(), _data_size };

	// if (offset <= 20 && offset + buffer.size() >= 21) {
	// 	printf("writing: %i\n", buffer.begin()[buffer.size() - offset]);
	// 	tmp[20 - offset] = _data_size;
	// }

	// // apply all layers
	// for (auto& layer : _layers) {
	// 	const auto size = layer->data_size();
	// 	layer->encode(content, data.subspan(data.size() - size), offset);
	// 	data = data.subspan(0, data.size() - size);
	// }

	// // forward to parent
	// _parent->write({ tmp.data(), buffer.size() }, offset);
	// printf("wrote offset: %lli, size: %zi\n", offset, buffer.size());
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
	return _data_manager.translate_file_size(_parent->file_size());
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

std::uint8_t layered_file::_data_size(const layers_type& layers) noexcept
{
	std::uint8_t data_size = 0;
	for (const auto& layer : layers) {
		data_size += layer->data_size();
	}
	return data_size;
}
