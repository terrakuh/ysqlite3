#ifndef YSQLITE3_VFS_LAYER_LAYERED_VFS_HPP_
#define YSQLITE3_VFS_LAYER_LAYERED_VFS_HPP_

#include "../vfs.hpp"
#include "layer.hpp"
#include "layered_file.hpp"

#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace ysqlite3 {
namespace vfs {
namespace layer {

template<typename VFS, typename File = layered_file,
         typename =
             std::enable_if<std::is_base_of<vfs, VFS>::value && std::is_base_of<layered_file, File>::value>>
class layered_vfs : public VFS
{
public:
	typedef File file_type;

	template<typename... Args>
	layered_vfs(Args&&... args) : VFS(std::forward<Args>(args)...)
	{}
	template<typename Type>
	typename std::enable_if<std::is_base_of<layer, Type>::value>::type add_layer()
	{
		if (this->is_registered()) {
			throw std::system_error{ errc::vfs_already_registered };
		}

		_layer_creators.push_back([] { return std::unique_ptr<layer>(new Type{}); });
	}
	std::unique_ptr<file> open(const char* name, file_format format, open_flag_type flags,
	                           open_flag_type& output_flags) override
	{
		auto tmp = VFS::open(name, format, flags, output_flags);
		return std::unique_ptr<file>{ new File{ format, std::move(tmp), create_layers() } };
	}
	layered_file::layers_type create_layers() const
	{
		layered_file::layers_type layers;
		layers.reserve(_layer_creators.size());

		for (auto& creator : _layer_creators) {
			layers.push_back(std::unique_ptr<layer>{ creator() });
		}

		return layers;
	}

private:
	std::vector<std::unique_ptr<layer> (*)()> _layer_creators;
};

} // namespace layer
} // namespace vfs
} // namespace ysqlite3

#endif
