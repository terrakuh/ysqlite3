#include "encrypted_vfs.hpp"
#include "encryption_context.hpp"
#include "sqlite3.h"

#include <cstring>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <memory>
#include <sstream>
#include <iomanip>
#include <vector>
#include <array>
#include <regex>


namespace ysqlite3
{

typedef decltype(sqlite3_vfs::xOpen) xopen_signature;

sqlite3_vfs encrypted_vfs;
xopen_signature xopen_base = nullptr;
std::regex path_pattern(R"((.*?)-([0-9a-fA-F]+)-(.*))");


class encrypted_sub_class
{
public:
	encrypted_sub_class(encryption_context * _context) : _derived_methods{}
	{
		_base_methods = nullptr;
		this->_context = _context;
		_flags = 0;
	}
	~encrypted_sub_class()
	{
		puts("destroy");
	}
	static int xopen_link(sqlite3_vfs * _vfs, const char * _name, sqlite3_file * _file, int _flags, int * _out_flags)
	{
		// Don't allow any non main db files
		if (!(_flags & SQLITE_OPEN_MAIN_DB)) {
			return SQLITE_MISUSE;
		}

		// Extract filename and context address
		std::string _path = _name;
		std::smatch _match;
		uintptr_t _address = 0;

		if (std::regex_search(_path, _match, path_pattern)) {
			std::stringstream _stream(_match[2].str());

			_stream >> std::hex >> _address;

			_path = _match[1].str() + _match[3].str();
		}

		// No context given
		if (!_address || _path.empty()) {
			return SQLITE_MISUSE;
		}

#if defined(PRINT_OUTPUT)
		printf("Open %s with encryption context at %p\n", _path.c_str(), _address);
#endif

		// Create context
		return (new(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(encrypted_sub_class)) encrypted_sub_class(reinterpret_cast<encryption_context*>(_address)))->xopen(_vfs, _path.c_str(), _file, _flags, _out_flags);
	}

private:
	enum FLAGS
	{
		F_HEADER_LOADED = 0x1,
		F_HEADER_MODIFIED = 0x2
	};

	constexpr static auto header_size = 100;

	const sqlite3_io_methods * _base_methods;
	sqlite3_io_methods _derived_methods;
	encryption_context * _context;
	/** Holds the SQLite3 header with a constant header size*/
	std::array<int8_t, header_size> _header;
	/** Buffer used for encryption. */
	std::vector<int8_t> _encryption_buffer;
	/** Holds same status information about this class. */
	int8_t _flags;

	int xopen(sqlite3_vfs * _vfs, const char * _name, sqlite3_file * _file, int _flags, int * _out_flags)
	{
		auto _result = xopen_base(_vfs, _name, _file, _flags, _out_flags);

		// Failed to open
		if (_result != SQLITE_OK) {
			// Delete this
			if (!_file->pMethods) {
				this->~encrypted_sub_class();
			}

			return _result;
		}

		_base_methods = _file->pMethods;

		// Override base methods
		_derived_methods = sqlite3_io_methods{ *_file->pMethods };

		_derived_methods.xClose = &encrypted_sub_class::xclose_link;
		_derived_methods.xSync = &encrypted_sub_class::xsync_link;
		_derived_methods.xRead = &encrypted_sub_class::xread_link;
		_derived_methods.xWrite = &encrypted_sub_class::xwrite_link;

		_file->pMethods = &_derived_methods;

		return SQLITE_OK;
	}
	int xclose(sqlite3_file * _file)
	{
		_file->pMethods = _base_methods;

		this->~encrypted_sub_class();

		return _file->pMethods->xClose(_file);
	}
	int xsync(sqlite3_file * _file, int _flags)
	{
#if defined(PRINT_OUTPUT)
		printf("[%p]: syncing\n", this);
#endif

		return _base_methods->xSync(_file, _flags);
	}
	int xread(sqlite3_file * _file, void * _buffer, int _size, sqlite3_int64 _offset)
	{
#if defined(PRINT_OUTPUT)
		printf("[%p]: read %i bytes from %lli\n", this, _size, _offset);
#endif

		// Read header only once
		if (!(_flags & F_HEADER_LOADED)) {
			auto _result = _base_methods->xRead(_file, _header.data(), header_size, 0);

			if (_result != SQLITE_OK) {
				return _result;
			}

			// Decrypt header
			if (!_context->decrypt(0, _header.data(), header_size)) {
				return SQLITE_IOERR;
			}

			_flags |= F_HEADER_LOADED;

#if defined(PRINT_OUTPUT)
			printf("[%p]: read header from disk\n", this);
#endif
		}

		// Read from header
		if (_size + _offset <= header_size) {
			std::memcpy(_buffer, _header.data() + _offset, _size);

			return SQLITE_OK;
		}

		auto _result = _base_methods->xRead(_file, _buffer, _size, _offset);

		// Decrypt
		if (!_context->decrypt(_offset / _size, _buffer, _size)) {
			return SQLITE_IOERR;
		}

		return _result;
	}
	int xwrite(sqlite3_file * _file, const void * _buffer, int _size, sqlite3_int64 _offset)
	{
#if defined(PRINT_OUTPUT)
		printf("[%p]: write %i bytes at %lli\n", this, _size, _offset);
#endif

		// Write into the SQLite header
		if (_size + _offset <= header_size) {
			std::memcpy(_header.data() + _offset, _buffer, _size);

			_flags |= F_HEADER_MODIFIED;

			return SQLITE_OK;
		}
		
		// Encrypt
		_encryption_buffer.resize(_size);

		if (!(_buffer = _context->encrypt(_offset / _size, _buffer, _size, _encryption_buffer.data()))) {
			return SQLITE_IOERR;
		}

		// Write
		return _base_methods->xWrite(_file, _buffer, _size, _offset);
	}
	static int xclose_link(sqlite3_file * _file)
	{
		return instance(_file)->xclose(_file);
	}
	static int xsync_link(sqlite3_file * _file, int _flags)
	{
		return instance(_file)->xsync(_file, _flags);
	}
	static int xread_link(sqlite3_file * _file, void * _buffer, int _size, sqlite3_int64 _offset)
	{
		return instance(_file)->xread(_file, _buffer, _size, _offset);
	}
	static int xwrite_link(sqlite3_file * _file, const void * _buffer, int _size, sqlite3_int64 _offset)
	{
		return instance(_file)->xwrite(_file, _buffer, _size, _offset);
	}
	static encrypted_sub_class * instance(sqlite3_file * _file)
	{
		return reinterpret_cast<encrypted_sub_class*>(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(encrypted_sub_class));
	}
};




void register_encrypted_vfs()
{
	sqlite3_initialize();

	// Get default VFS
	auto _default = sqlite3_vfs_find(nullptr);

	xopen_base = _default->xOpen;

	std::memcpy(&encrypted_vfs, _default, sizeof(encrypted_vfs));

	encrypted_vfs.szOsFile += sizeof(encrypted_sub_class);
	encrypted_vfs.pNext = nullptr;
	encrypted_vfs.zName = SQLITE3_ENCRYPTED_VFS_NAME;
	encrypted_vfs.xOpen = &encrypted_sub_class::xopen_link;
	
	sqlite3_vfs_register(&encrypted_vfs, 0);
}

void unregister_encrypted_vfs()
{
	sqlite3_vfs_unregister(&encrypted_vfs);
}

}