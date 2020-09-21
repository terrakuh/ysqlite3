#ifndef YSQLITE3_VFS_STREAMING_VFS_HPP_
#define YSQLITE3_VFS_STREAMING_VFS_HPP_

#include "vfs.hpp"

#include <istream>
#include <memory>

namespace ysqlite3 {
namespace vfs {

class streaming_vfs : public vfs
{
public:
	streaming_vfs(std::shared_ptr<std::iostream> stream);
};

} // namespace vfs
} // namespace ysqlite3

#endif
