#include "encrypted_vfs.hpp"

#include <cstring>
#include <cstdint>


namespace sqlite3
{

#include "sqlite3.h"

typedef decltype(sqlite3_vfs::xOpen) xopen_signature;
typedef const sqlite3_io_methods * base_method_type;

sqlite3_vfs encrypted_vfs;

// The base function of the default VFS
xopen_signature xopen_base = nullptr;


inline int xclose_derived(sqlite3_file * _file)
{
	auto _base_methods = reinterpret_cast<base_method_type>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*));

	// Delete custom method table
	delete _file->pMethods;
	_file->pMethods = _base_methods;

	return _base_methods->xClose(_file);
}

inline int xread_derived(sqlite3_file * _file, void * _buffer, int _size, sqlite3_int64 _offset)
{
	auto _base = reinterpret_cast<base_method_type>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*))->xRead;
	auto _result = _base(_file, _buffer, _size, _offset);

	// Decrypt

	return _result;
}

inline int xwrite_derived(sqlite3_file * _file, const void * _buffer, int _size, sqlite3_int64 _offset)
{
	// Encrypt

	auto _base = reinterpret_cast<base_method_type>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*))->xWrite;

	return _base(_file, _buffer, _size, _offset);
}

inline int xopen_derived(sqlite3_vfs * _vfs, const char * _name, sqlite3_file * _file, int _flags, int * _out_flags)
{


	auto _result = xopen_base(_vfs, _name, _file, _flags, _out_flags);

	// Override I/O functions
	if (_file->pMethods) {
		auto _methods = new sqlite3_io_methods(*_file->pMethods);

		// Save base methods
		*reinterpret_cast<const void**>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*)) = _file->pMethods;

		_methods->xClose = &xclose_derived;
		_methods->xRead = &xread_derived;
		_methods->xWrite = &xwrite_derived;

		_file->pMethods = _methods;
	} else {
		std::memset(reinterpret_cast<int8_t*>(_file + encrypted_vfs.szOsFile - sizeof(void*) * 2), 0, sizeof(void*) * 2);
	}

	return _result;
}

void register_encrypted_vfs()
{
	// Get default VFS
	auto _default = sqlite3_vfs_find(nullptr);

	std::memcpy(&encrypted_vfs, _default, sizeof(encrypted_vfs));

	encrypted_vfs.szOsFile += sizeof(void*);
	encrypted_vfs.pNext = nullptr;
	encrypted_vfs.zName = SQLITE3_ENCRYPTED_VFS_NAME;
	encrypted_vfs.xOpen = &xopen_derived;

	sqlite3_vfs_register(&encrypted_vfs, 0);
}

void unregister_encrypted_vfs()
{
	sqlite3_vfs_unregister(&encrypted_vfs);
}

}