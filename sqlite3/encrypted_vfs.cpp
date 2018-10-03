#include "encrypted_vfs.hpp"
#include "encryption_context.hpp"

#include <cstring>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <memory>
#include <sstream>
#include <iomanip>


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
	auto _base_methods = *reinterpret_cast<base_method_type*>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*));

	// Delete custom method table
	delete _file->pMethods;
	_file->pMethods = _base_methods;

	return _base_methods->xClose(_file);
}

inline int xread_base(sqlite3_file * _file, void * _buffer, int _size, sqlite3_int64 _offset)
{
	auto _base = (*reinterpret_cast<base_method_type*>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*)))->xRead;

	return _base(_file, _buffer, _size, _offset);
}

inline int xread_derived(sqlite3_file * _file, void * _buffer, int _size, sqlite3_int64 _offset)
{
	auto _context = *reinterpret_cast<encryption_context**>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*) * 2);
	auto _result = xread_base(_file, _buffer, _size, _offset);

	printf("[%p]: read %i bytes from %lli\n", _file, _size, _offset);

	/*if (_size % 4096) {
		for (int i = 0; i < _size; ++i) {
			printf("%02x ", reinterpret_cast<uint8_t*>(_buffer)[i]);
			if ((i + 1) % 16 == 0) {
				puts("");
			}
		}
		if (_size % 16) {
			puts("");
		}
	}*/

	// Decrypt
	_context->decrypt(_offset / _size, _buffer, _size, _buffer, std::bind(&xread_base, _file, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

	return _result;
}

inline int xwrite_base(sqlite3_file * _file, const void * _buffer, int _size, sqlite3_int64 _offset)
{
	auto _base = (*reinterpret_cast<base_method_type*>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*)))->xWrite;

	return _base(_file, _buffer, _size, _offset);
}

inline int xwrite_derived(sqlite3_file * _file, const void * _buffer, int _size, sqlite3_int64 _offset)
{
	auto _context = *reinterpret_cast<encryption_context**>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*) * 2);

	// Encrypt
	std::unique_ptr<int8_t[]> _encrypted(new int8_t[_size]);

	printf("[%p]: write %i bytes at %lli sectorsize: %i\n", _file, _size, _offset, _file->pMethods->xSectorSize(_file));

	_context->encrypt(_offset / _size, _buffer, _size, _encrypted.get());
	
	// Write
	return xwrite_base(_file, _encrypted.get(), _size, _offset /* + TODO*/);
}

inline int xsync_base(sqlite3_file * _file, int _flags)
{
	auto _base = (*reinterpret_cast<base_method_type*>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*)))->xSync;

	return _base(_file, _flags);
}

inline int xsync_derived(sqlite3_file * _file, int _flags)
{
	auto _context = *reinterpret_cast<encryption_context**>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*) * 2);

	printf("[%p]: syncing\n", _file);

	// Get all overhead pages
	for (auto & _sector : _context->modified_overhead_sectors()) {
		auto _result = xwrite_base(_file, std::get<0>(_sector), std::get<1>(_sector), std::get<2>(_sector));

		if (_result != SQLITE_OK) {
			return _result;
		}
	}

	return xsync_base(_file, _flags);
}

inline int xopen_derived(sqlite3_vfs * _vfs, const char * _name, sqlite3_file * _file, int _flags, int * _out_flags)
{
	// Don't allow any non main db files
	if (!(_flags & SQLITE_OPEN_MAIN_DB)) {
		return SQLITE_MISUSE;
	}

	// Extract filename and context address
	auto _address = std::strrchr(_name, '/');

	if (!_address) {
		_address = std::strrchr(_name, '\\');

		if (!_address) {
			_address = _name;
		} else {
			++_address;
		}
	} else {
		++_address;
	}

	intptr_t _addr = 0;
	std::string _real_path(_name, _address);

	{
		std::stringstream _s(_address);

		_s >> std::hex >> _addr;

		_real_path.append(_address + _s.tellg() + 1);
	}

	_name = _real_path.c_str();

	auto _result = xopen_base(_vfs, _name, _file, _flags, _out_flags);

	// Override I/O functions
	if (_file->pMethods) {
		auto _methods = new sqlite3_io_methods(*_file->pMethods);

		// Save context and base methods
		*reinterpret_cast<encryption_context**>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*) * 2) = reinterpret_cast<encryption_context*>(_addr);
		*reinterpret_cast<base_method_type*>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*)) = _file->pMethods;

		printf("open %s with %s and save to %p\n", _name, _vfs->zName, _file);

		_methods->xClose = &xclose_derived;
		_methods->xRead = &xread_derived;
		_methods->xWrite = &xwrite_derived;
		_methods->xSync = &xsync_derived;

		_file->pMethods = _methods;
	} else {
		std::memset(reinterpret_cast<int8_t*>(_file + encrypted_vfs.szOsFile - sizeof(void*) * 2), 0, sizeof(void*) * 2);
	}

	return _result;
}

void register_encrypted_vfs()
{
	sqlite3_initialize();
	// Get default VFS
	auto _default = sqlite3_vfs_find(nullptr);

	xopen_base = _default->xOpen;

	std::memcpy(&encrypted_vfs, _default, sizeof(encrypted_vfs));

	encrypted_vfs.szOsFile += sizeof(void*) * 2;
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