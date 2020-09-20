#ifndef YSQLITE3_VFS_LAYER_ENCRYPTOR_HPP_
#define YSQLITE3_VFS_LAYER_ENCRYPTOR_HPP_

namespace ysqlite3 {
namespace vfs {
namespace layer {

class encryptor
{
public:
	virtual ~encryptor() = default;
	virtual void encrypt() = 0;
	virtual void decrypt() = 0;
};

} // namespace layer
} // namespace vfs
} // namespace ysqlite3

#endif // !YSQLITE3_VFS_LAYER_ENCRYPTOR_HPP_
