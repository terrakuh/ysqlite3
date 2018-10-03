#include "encrypted_vfs.hpp"
#include "encryption_context.hpp"

#include <cstring>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <memory>
#include <sstream>
#include <iomanip>

#define PRINT_OUTPUT


namespace sqlite3
{

#include "sqlite3.h"

typedef decltype(sqlite3_vfs::xOpen) xopen_signature;
typedef const sqlite3_io_methods * base_method_type;

sqlite3_vfs encrypted_vfs;

// The base function of the default VFS
xopen_signature xopen_base = nullptr;
constexpr auto sqlite_header_size = 100;
constexpr auto header_control_byte = -1;
constexpr auto header_read = 0x1;
constexpr auto header_modified = 0x2;


inline int xclose_derived(sqlite3_file * _file)
{
	auto _header = *reinterpret_cast<int8_t**>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*) * 3) - 1;
	auto _base_methods = *reinterpret_cast<base_method_type*>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*));

	// Delete custom method table
	delete[] _header;
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
#if defined(PRINT_OUTPUT)
	printf("[%p]: read %i bytes from %lli\n", _file, _size, _offset);
#endif

	auto _header = *reinterpret_cast<int8_t**>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*) * 3);

	// Read header
	if (!(_header[header_control_byte] & header_read)) {
		auto _result = xread_base(_file, _header, sqlite_header_size, 0);

		if (_result != SQLITE_OK) {
			return _result;
		}

		_header[header_control_byte] |= header_read;
	}

	// Read from header
	if (_size + _offset <= sqlite_header_size) {
		std::memcpy(_buffer, _header + _offset, _size);

		return SQLITE_OK;
	}

	auto _context = *reinterpret_cast<encryption_context**>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*) * 2);
	auto _result = xread_base(_file, _buffer, _size, _offset);

	// Decrypt
	if (_context->does_something()) {
		_context->decrypt(_offset / _size, _buffer, _size);
	}

	return _result;
}

inline int xwrite_base(sqlite3_file * _file, const void * _buffer, int _size, sqlite3_int64 _offset)
{
	auto _base = (*reinterpret_cast<base_method_type*>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*)))->xWrite;

	return _base(_file, _buffer, _size, _offset);
}

inline int xwrite_derived(sqlite3_file * _file, const void * _buffer, int _size, sqlite3_int64 _offset)
{
#if defined(PRINT_OUTPUT)
	printf("[%p]: write %i bytes at %lli\n", _file, _size, _offset);
#endif

	// Write into the SQLite header
	if (_size + _offset <= sqlite_header_size) {
		auto _header = *reinterpret_cast<int8_t**>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*) * 3);

		std::memcpy(_header + _offset, _buffer, _size);
		_header[header_control_byte] |= header_modified;

		return SQLITE_OK;
	}

	auto _context = *reinterpret_cast<encryption_context**>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*) * 2);
	std::unique_ptr<int8_t[]> _encrypted;

	// Encrypt
	if (_context->does_something()) {
		_encrypted.reset(new int8_t[_size]);

		_context->encrypt(_offset / _size, _buffer, _size, _encrypted.get());

		_buffer = _encrypted.get();
	}

	// Write
	return xwrite_base(_file, _buffer, _size, _offset);
}

inline int xsync_base(sqlite3_file * _file, int _flags)
{
	auto _base = (*reinterpret_cast<base_method_type*>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*)))->xSync;

	return _base(_file, _flags);
}

inline int xsync_derived(sqlite3_file * _file, int _flags)
{
#if defined(PRINT_OUTPUT)
	printf("[%p]: syncing\n", _file);
#endif

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
		auto _context = reinterpret_cast<encryption_context*>(_addr);

		// Save context and base methods
		*reinterpret_cast<int8_t**>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*) * 3) = new int8_t[sqlite_header_size + 1]{} + 1;
		*reinterpret_cast<encryption_context**>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*) * 2) = _context;
		*reinterpret_cast<base_method_type*>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(void*)) = _file->pMethods;

#if defined(PRINT_OUTPUT)
		printf("open %s with %s and save to %p\n", _name, _vfs->zName, _file);
#endif

		_methods->xClose = &xclose_derived;
		_methods->xRead = &xread_derived;
		_methods->xWrite = &xwrite_derived;
		_methods->xSync = &xsync_derived;

		_file->pMethods = _methods;
	} else {
		std::memset(reinterpret_cast<int8_t*>(_file + encrypted_vfs.szOsFile - sizeof(void*) * 3), 0, sizeof(void*) * 3);
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

	encrypted_vfs.szOsFile += sizeof(void*) * 3;
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