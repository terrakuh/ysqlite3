#ifndef YSQLITE3_VFS_LAYER_LAYER_HPP_
#define YSQLITE3_VFS_LAYER_LAYER_HPP_

#include "../../sqlite3.h"

#include <gsl/gsl>
#include <vector>

namespace ysqlite3 {
namespace vfs {
namespace layer {

class layer
{
public:
	virtual ~layer()                                                       = default;
	virtual void encode(gsl::span<gsl::byte> buffer, sqlite3_int64 offset) = 0;
	virtual void decode(gsl::span<gsl::byte> buffer, sqlite3_int64 offset) = 0;
};

} // namespace layer
} // namespace vfs
} // namespace ysqlite3

#endif // !YSQLITE3_VFS_LAYER_LAYER_HPP_
