#ifndef YSQLITE3_VFS_LAYER_LAYERED_VFS_HPP_
#define YSQLITE3_VFS_LAYER_LAYERED_VFS_HPP_

#include "../vfs.hpp"
#include "layer.hpp"
#include "layered_file.hpp"

#include <gsl/gsl>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace ysqlite3 {
namespace vfs {
namespace layer {

template<typename Vfs, typename File = layered_file,
         typename = std::enable_if<std::is_base_of<vfs, Vfs>::value && std::is_base_of<layered_file, File>::value>>
class layered_vfs : public Vfs
{
public:
	typedef File file_type;

	template<typename... Args>
	layered_vfs(Args&&... args) : Vfs(std::forward<Args>(args)...)
	{}
	template<typename T>
	typename std::enable_if<std::is_base_of<layer, T>::value>::type add_layer()
	{
		Expects(!this->registered());

		_layer_creators.push_back(_creator<T>);
	}
	virtual gsl::not_null<gsl::owner<file*>> open(gsl::czstring<> name, file::format format,
	                                              database::open_flag_type flags,
	                                              database::open_flag_type& output_flags) override
	{
		auto v = Vfs::open(name, format, flags, output_flags);

		return new File(format, v, create_layers());
	}
	virtual layered_file::layers_type create_layers() const
	{
		layered_file::layers_type layers;

		layers.reserve(_layer_creators.size());

		for (auto& creator : _layer_creators) {
			layers.push_back(std::unique_ptr<layer>(creator()));
		}

		return layers;
	}

private:
	typedef std::unique_ptr<layer> (*creator_type)();

	std::vector<creator_type> _layer_creators;

	template<typename T>
	static std::unique_ptr<layer> _creator()
	{
		return std::unique_ptr<layer>(new T());
	}
};

} // namespace layer
} // namespace vfs
} // namespace ysqlite3

#endif