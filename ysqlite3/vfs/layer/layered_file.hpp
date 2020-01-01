#ifndef YSQLITE3_VFS_LAYER_LAYERED_FILE_HPP_
#define YSQLITE3_VFS_LAYER_LAYERED_FILE_HPP_

#include "../file.hpp"
#include "layer.hpp"

#include <vector>

namespace ysqlite3 {
namespace vfs {
namespace layer {

class layered_file : public file
{
public:
	typedef std::vector<std::unique_ptr<layer>> layers_type;

	layered_file(gsl::not_null<gsl::owner<file*>> parent, layers_type layers) noexcept
	    : _parent(parent), _layers(std::move(layers))
	{}
	virtual void close() override
	{
		_parent->close();
	}
	virtual void read(gsl::span<gsl::byte> buffer, sqlite3_int64 offset) override
	{
		// read from parent
		_parent->read(buffer, offset);

		// remove all layers
		for (auto i = _layers.rbegin(); i != _layers.rend(); ++i) {
			auto& layer = *i;

			layer->decode(buffer, offset);
		}
	}
	virtual void write(gsl::span<const gsl::byte> buffer, sqlite3_int64 offset) override
	{
		std::unique_ptr<gsl::byte[]> tmp(new gsl::byte[buffer.size()]);

		std::memcpy(tmp.get(), buffer.data(), buffer.size());

		// apply all layers
		for (auto& layer : _layers) {
			layer->encode({ tmp.get(), buffer.size() }, offset);
		}

		// forward to parent
		_parent->write({ tmp.get(), buffer.size() }, offset);
	}
	virtual void truncate(sqlite3_int64 size) override
	{
		_parent->truncate(size);
	}
	virtual void sync(sync_flag flag) override
	{
		_parent->sync(flag);
	}
	virtual sqlite3_int64 file_size() const override
	{
		return _parent->file_size();
	}
	virtual void lock(lock_flag flag) override
	{
		_parent->lock(flag);
	}
	virtual void unlock(lock_flag flag) override
	{
		_parent->unlock(flag);
	}
	virtual bool has_reserved_lock() const override
	{
		return _parent->has_reserved_lock();
	}
	virtual void file_control(file_cntl operation, void* arg) override
	{
		_parent->file_control(operation, arg);
	}
	virtual int sector_size() const noexcept override
	{
		return _parent->sector_size();
	}
	virtual int device_characteristics() const noexcept override
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

#endif // !YSQLITE3_VFS_LAYER_LAYERED_FILE_HPP_
