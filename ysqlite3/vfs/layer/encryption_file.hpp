#ifndef YSQLITE3_VFS_LAYER_ENCRYPTION_FILE_HPP_
#define YSQLITE3_VFS_LAYER_ENCRYPTION_FILE_HPP_

#include "layered_file.hpp"

#include <cstring>

namespace ysqlite3 {
namespace vfs {
namespace layer {

class encryption_file : public layered_file
{
public:
	using layered_file::layered_file;

	virtual void file_control(file_cntl operation, void* arg) override
	{
		if (operation == file_cntl_pragma) {
			auto name  = static_cast<char**>(arg)[1];
			auto value = static_cast<char**>(arg)[2];

			if (!std::strcmp(name, "key")) {
				return;
			} else if (!std::strcmp(name, "key_b64")) {
				return;
			}
		}

		layered_file::file_control(operation, arg);
	}
};

} // namespace layer
} // namespace vfs
} // namespace ysqlite3

#endif // !YSQLITE3_VFS_LAYER_ENCRYPTION_FILE_HPP_
