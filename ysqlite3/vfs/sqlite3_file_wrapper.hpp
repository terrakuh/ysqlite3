#ifndef YSQLITE3_VFS_SQLITE3_FILE_HPP_
#define YSQLITE3_VFS_SQLITE3_FILE_HPP_

#include "../sqlite3.h"
#include "file.hpp"

#include <memory>

namespace ysqlite3 {
namespace vfs {

/// Wraps a standard file implementation from SQLite3 into the File interface.
class SQLite3_file_wrapper : public File
{
public:
	SQLite3_file_wrapper(const char* name, File_format format, std::shared_ptr<sqlite3_file> parent) noexcept;
	void close() override;
	void read(std::span<std::byte> buffer, sqlite3_int64 offset) override;
	void write(std::span<const std::byte> buffer, sqlite3_int64 offset) override;
	void truncate(sqlite3_int64 size) override;
	void sync(Sync_flag flag) override;
	sqlite3_int64 file_size() const override;
	void lock(Lock_flag flag) override;
	void unlock(Lock_flag flag) override;
	bool has_reserved_lock() const override;
	void file_control(File_control operation, void* arg) override;
	int sector_size() const noexcept override;
	int device_characteristics() const noexcept override;
	void shm_map(int page, int page_size, bool is_write, void volatile** mapped_memory) override;
	void shm_lock(int offset, int n, int flags) override;
	void shm_barrier() noexcept override;
	void shm_unmap(int delete_flag) override;
	void fetch(sqlite3_int64 offset, int amount, void** buffer) override;
	void unfetch(sqlite3_int64 offset, void* buffer) override;

private:
	std::shared_ptr<sqlite3_file> _parent;
};

} // namespace vfs
} // namespace ysqlite3

#endif
