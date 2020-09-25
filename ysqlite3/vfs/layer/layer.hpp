#ifndef YSQLITE3_VFS_LAYER_LAYER_HPP_
#define YSQLITE3_VFS_LAYER_LAYER_HPP_

#include "../../span.hpp"
#include "../../sqlite3.h"
#include "../file.hpp"

#include <cstdint>

namespace ysqlite3 {
namespace vfs {
namespace layer {

class layer
{
public:
	virtual ~layer()                                                              = default;
	virtual void encode(span<std::uint8_t*> page, span<std::uint8_t*> data)       = 0;
	virtual void decode(span<std::uint8_t*> page, span<const std::uint8_t*> data) = 0;
	virtual std::uint8_t data_size() const noexcept
	{
		return 0;
	}
	virtual bool pragma(const char* name, const char* value)
	{
		return false;
	}
};

} // namespace layer
} // namespace vfs
} // namespace ysqlite3

#endif
