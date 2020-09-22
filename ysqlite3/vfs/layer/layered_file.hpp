#ifndef YSQLITE3_VFS_LAYER_LAYERED_FILE_HPP_
#define YSQLITE3_VFS_LAYER_LAYERED_FILE_HPP_

#include "../file.hpp"
#include "layer.hpp"

#include <cstring>
#include <memory>
#include <vector>

namespace ysqlite3 {
namespace vfs {
namespace layer {

class layered_file : public file
{
public:
	typedef std::vector<std::unique_ptr<layer>> layers_type;

	layered_file(file_format format, std::unique_ptr<file> parent, layers_type layers) noexcept
	    : file{ format }, _parent{ std::move(parent) }, _layers{ std::move(layers) }
	{}
	void close() override
	{
		_parent->close();
	}
	void read(span<std::uint8_t*> buffer, sqlite3_int64 offset) override
	{
		// read from parent
		_parent->read(buffer, offset);

		// remove all layers
		for (auto i = _layers.rbegin(); i != _layers.rend(); ++i) {
			(*i)->decode(buffer, offset);
		}
	}
	void write(span<const std::uint8_t*> buffer, sqlite3_int64 offset) override
	{
		std::vector<std::uint8_t> tmp;
		tmp.resize(buffer.size());
		std::memcpy(tmp.data(), buffer.begin(), buffer.size());

		// apply all layers
		for (auto& layer : _layers) {
			layer->encode({ tmp.data(), buffer.size() }, offset);
		}

		// forward to parent
		_parent->write({ tmp.data(), buffer.size() }, offset);
	}
	void truncate(sqlite3_int64 size) override
	{
		_parent->truncate(size);
	}
	void sync(sync_flag flag) override
	{
		_parent->sync(flag);
	}
	sqlite3_int64 file_size() const override
	{
		return _parent->file_size();
	}
	void lock(lock_flag flag) override
	{
		_parent->lock(flag);
	}
	void unlock(lock_flag flag) override
	{
		_parent->unlock(flag);
	}
	bool has_reserved_lock() const override
	{
		return _parent->has_reserved_lock();
	}
	void file_control(file_cntl operation, void* arg) override
	{
		_parent->file_control(operation, arg);
	}
	int sector_size() const noexcept override
	{
		return _parent->sector_size();
	}
	int device_characteristics() const noexcept override
	{
		return _parent->device_characteristics();
	}

private:
	std::unique_ptr<file> _parent;
	layers_type _layers;
};

} // namespace layer
} // namespace vfs
} // namespace ysqlite3

#endif
