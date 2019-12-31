#pragma once

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

template<typename File = layered_file>
class layered_vfs : public vfs<File>
{
public:
	layered_vfs(gsl::not_null<gsl::shared_ptr<vfs>> parent, gsl::not_null<gsl::czstring<>> name)
	    : vfs(name), _parent(std::move(parent))
	{}
	template<typename T>
	typename std::enable_if<std::is_base_of<layer, T>::value>::type add_layer()
	{
		Expects(!registered());

		_layer_creators.push_back(_creator<T>);
	}

	virtual int max_pathname() const noexcept override
	{
		return _parent->max_pathname();
	}
	virtual gsl::not_null<gsl::owner<file*>> open(gsl::czstring<> name, database::open_flag_type flags,
	                                              database::open_flag_type& output_flags) override
	{
		auto f = _parent->open(name, flags, output_flags);

		// wrap file
		return new layered_file(std::shared_ptr<file>(f.get()), create_layers());
	}
	virtual void delete_file(gsl::czstring<> name, bool sync_directory) override
	{
		_parent->delete_file(name, sync_directory);
	}
	virtual bool access(gsl::czstring<> name, access_flag flag) override
	{
		return _parent->access(name, flag);
	}
	virtual void full_pathname(gsl::czstring<> input, gsl::string_span<> output) override
	{
		_parent->full_pathname(input, output);
	}
	virtual void* dlopen(gsl::czstring<> filename) noexcept override
	{
		return _parent->dlopen(filename);
	}
	virtual void dlerror(gsl::string_span<> buffer) noexcept override
	{
		_parent->dlerror(buffer);
	}
	virtual dlsym_type dlsym(gsl::not_null<void*> handle, gsl::czstring<> symbol) noexcept override
	{
		return _parent->dlsym(handle, symbol);
	}
	virtual void dlclose(gsl::not_null<void*> handle) noexcept override
	{
		_parent->dlclose(handle);
	}
	virtual int random(gsl::span<gsl::byte> buffer) noexcept override
	{
		return _parent->random(buffer);
	}
	virtual sleep_duration_type sleep(sleep_duration_type time) noexcept override
	{
		return _parent->sleep(time);
	}
	virtual time_type current_time() override
	{
		return _parent->current_time();
	}
	virtual int last_error(gsl::string_span<> buffer) noexcept override
	{
		return _parent->last_error(buffer);
	}
	virtual int set_system_call(gsl::czstring<> name, sqlite3_syscall_ptr system_call) noexcept override
	{
		return _parent->set_system_call(name, system_call);
	}
	virtual sqlite3_syscall_ptr get_system_call(gsl::czstring<> name) noexcept override
	{
		return _parent->get_system_call(name);
	}
	virtual gsl::czstring<> next_system_call(gsl::czstring<> name) noexcept override
	{
		return _parent->next_system_call(name);
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

	std::shared_ptr<vfs> _parent;
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