#ifndef YSQLITE3_VFS_LAYER_LAYERED_FILE_HPP_
#define YSQLITE3_VFS_LAYER_LAYERED_FILE_HPP_

#include "../file.hpp"
#include "data_manager.hpp"
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

	layered_file(file_format format, std::unique_ptr<file> parent, layers_type layers) noexcept;
	void close() override;
	void read(span<std::uint8_t*> buffer, sqlite3_int64 offset) override;
	void write(span<const std::uint8_t*> buffer, sqlite3_int64 offset) override;
	void truncate(sqlite3_int64 size) override;
	void sync(sync_flag flag) override;
	sqlite3_int64 file_size() const override;
	void lock(lock_flag flag) override;
	void unlock(lock_flag flag) override;
	bool has_reserved_lock() const override;
	void file_control(file_cntl operation, void* arg) override;
	int sector_size() const noexcept override;
	int device_characteristics() const noexcept override;

private:
	std::unique_ptr<file> _parent;
	layers_type _layers;
	data_manager _data_manager;

	static std::uint8_t _data_size(const layers_type& layers) noexcept;
};

} // namespace layer
} // namespace vfs
} // namespace ysqlite3

#endif
