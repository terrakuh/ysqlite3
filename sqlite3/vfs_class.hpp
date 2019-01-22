#pragma once

#include <cstdio>
#include <functional>
#include <vector>
#include <type_traits>
#include <stdexcept>

#include "sqlite3.h"
#include "config.hpp"

#define SQLITE3_CUSTOM_VFS_NAME "ycvfs"


namespace ysqlite3
{

class vfs_class
{
public:
	typedef decltype(sqlite3_vfs::xOpen) xopen_signature_t;


	vfs_class() noexcept;
	virtual ~vfs_class() noexcept;
	static void register_vfs();
	template<typename Type>
	static typename std::enable_if<std::is_base_of<vfs_class, Type>::value>::type register_vfs_class()
	{
		if (_xopen_base) {
			throw std::runtime_error("vfs was already registered.");
		}

		_factories.push_back([](void * _buffer) { new(_buffer) Type(); });
	}
	static int xopen_link(sqlite3_vfs * _vfs, const char * _name, sqlite3_file * _file, int _flags, int * _out_flags);

protected:
	typedef std::function<void(void*)> factory_t;

	class factory_error : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	static bool little_endian() noexcept;
	virtual int xopen(sqlite3_vfs * _vfs, const char * _name, sqlite3_file * _file, int _flags, int * _out_flags);
	virtual int xclose(sqlite3_file * _file);
	virtual int xsync(sqlite3_file * _file, int _flags);
	virtual int xread(sqlite3_file * _file, void * _buffer, int _size, sqlite3_int64 _offset);
	virtual int xwrite(sqlite3_file * _file, const void * _buffer, int _size, sqlite3_int64 _offset);
	virtual int xfile_control(sqlite3_file * _file, int _op, void * _arg);
	template<typename Type>
	static Type read(const void * _buffer)
	{
		if (little_endian()) {
			Type _value = Type();
			auto _ptr = reinterpret_cast<const uint8_t*>(_buffer);

			for (auto i = 0; i < sizeof(Type); ++i) {
				_value = static_cast<Type>(_value | static_cast<Type>(_ptr[i]) << 8 * (sizeof(Type) - i - 1));
			}

			return _value;
		}

		return *reinterpret_cast<const Type*>(_buffer);
	}

private:
	static xopen_signature_t _xopen_base;
	static sqlite3_vfs _vfs;
	static size_t _max_class_size;
	static std::vector<factory_t> _factories;
	const sqlite3_io_methods * _base_methods;
	sqlite3_io_methods _derived_methods;

	static int xclose_link(sqlite3_file * _file);
	static int xsync_link(sqlite3_file * _file, int _flags);
	static int xread_link(sqlite3_file * _file, void * _buffer, int _size, sqlite3_int64 _offset);
	static int xwrite_link(sqlite3_file * _file, const void * _buffer, int _size, sqlite3_int64 _offset);
	static int xfile_control_link(sqlite3_file * _file, int _op, void * _arg);
	static vfs_class * instance(sqlite3_file * _file);
};

}