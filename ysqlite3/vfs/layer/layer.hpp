#ifndef YSQLITE3_VFS_LAYER_LAYER_HPP_
#define YSQLITE3_VFS_LAYER_LAYER_HPP_

#include "../../span.hpp"
#include "../../sqlite3.h"

#include <cstdint>

namespace ysqlite3 {
namespace vfs {
namespace layer {

class layer
{
public:
	virtual ~layer()                                                      = default;
	virtual void encode(span<std::uint8_t*> buffer, sqlite3_int64 offset) = 0;
	virtual void decode(span<std::uint8_t*> buffer, sqlite3_int64 offset) = 0;
};

} // namespace layer
} // namespace vfs
} // namespace ysqlite3

#endif
