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
	encrypted_sub_class(const std::shared_ptr<encryption_context> & _context) : _derived_methods{}, _context(_context)
	{
#if defined(PRINT_OUTPUT)
		printf("[%p]: creating\n", this);
#endif

		_base_methods = nullptr;
		_page_size = 0;
	}
	~encrypted_sub_class()
	{
#if defined(PRINT_OUTPUT)
		printf("[%p]: destroying\n", this);
#endif
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

		auto & _context = *reinterpret_cast<const std::shared_ptr<encryption_context>*>(_address);

#if defined(PRINT_OUTPUT)
		printf("Open %s with encryption context at %p\n", _path.c_str(), _context.get());
#endif

		// Create context
		auto _sub_class = new(reinterpret_cast<int8_t*>(_file) + encrypted_vfs.szOsFile - sizeof(encrypted_sub_class)) encrypted_sub_class(_context);

		return _sub_class->xopen(_vfs, _path.c_str(), _file, _flags, _out_flags);
	}

private:
	constexpr static auto header_size = 100;

	const sqlite3_io_methods * _base_methods;
	sqlite3_io_methods _derived_methods;
	std::shared_ptr<encryption_context> _context;
	/** Buffer used for encryption. */
	std::vector<int8_t> _encryption_buffer;
	/** The page size. */
	uint16_t _page_size;

	void update_parameter(const int8_t * _buffer, int _size, sqlite3_int64 _offset)
	{
		// Page size updated
		if (_offset <= 16 && _offset + _size >= 18) {
			_page_size = read<uint16_t>(_buffer + 16 - _offset);

#if defined(PRINT_OUTPUT)
			printf("[%p]: loaded page size at %i\n", this, static_cast<int>(_page_size));
#endif
		}
	}
	static bool little_endian() noexcept
	{
		int _test = 1;

		return *reinterpret_cast<char*>(&_test);
	}
	int read_from_header(sqlite3_file * _file, int8_t * _output, int _size, sqlite3_int64 _offset)
	{
		auto _result = _base_methods->xRead(_file, _output, _size, _offset);

		if (_result != SQLITE_OK) {
			return _result;
		}

		// Update parameters
		update_parameter(_output, _size, _offset);

		// Load and overwrite app data
		if (_offset == 0) {
			_context->load_app_data(_output);

			std::memcpy(_output, "SQLite format 3", 16);
		}

		return SQLITE_OK;
	}
	int read_encrypted_page(encryption_context::id_t _page_id, sqlite3_file * _file, int8_t * _output, int _size, sqlite3_int64 _offset)
	{
#if defined(PRINT_OUTPUT)
		printf("[%p]: encrypted read %i bytes from %lli\n", this, _size, _offset);
#endif

		// Reserve space
		_encryption_buffer.resize(_size);

		// Read encrypted page
		auto _result = _base_methods->xRead(_file, _encryption_buffer.data(), _size, _offset);

		if (_result != SQLITE_OK) {
			return _result;
		}

		// Special page because of header
		if (_page_id == 0) {
			update_parameter(_encryption_buffer.data(), _size, _offset);

			_context->load_app_data(_encryption_buffer.data());
		}

		// Decrypt
		_size -= SQLITE3_MAX_USER_DATA_SIZE;

		if (!_context->decrypt(_page_id, _encryption_buffer.data(), _size, _output, _encryption_buffer.data() + _size)) {
			return SQLITE_IOERR_AUTH;
		}

		// Overwrite app data for SQLite
		if (_page_id == 0) {
			std::memcpy(_output, "SQLite format 3", 16);
		}

		return SQLITE_OK;
	}
	int write_encrypted_page(encryption_context::id_t _page_id, sqlite3_file * _file, const int8_t * _input, sqlite3_int64 _offset)
	{
		_encryption_buffer.resize(header_size);

		// Special page because of header
		if (_page_id == 0) {
			update_parameter(_input, header_size, _offset);

			// Store app data
			_context->store_app_data(_encryption_buffer.data());
		}

#if defined(PRINT_OUTPUT)
		printf("[%p]: encrypted write %i bytes to %lli\n", this, _page_size, _offset);
#endif

		// Reserve space
		_encryption_buffer.resize(_page_size);

		// Encrypt
		auto _size = _page_size - SQLITE3_MAX_USER_DATA_SIZE;

		if (!_context->encrypt(_page_id, _input, _size, _encryption_buffer.data(), _encryption_buffer.data() + _size)) {
			return SQLITE_IOERR_AUTH;
		}

		// Write encrypted page to file
		return _base_methods->xWrite(_file, _encryption_buffer.data(), _size + SQLITE3_MAX_USER_DATA_SIZE, _offset);
	}
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
		_derived_methods.xFileControl = &encrypted_sub_class::xfile_control_link;

		_file->pMethods = &_derived_methods;

		return SQLITE_OK;
	}
	int xclose(sqlite3_file * _file)
	{
#if defined(PRINT_OUTPUT)
		printf("[%p]: closing\n", this);
#endif

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

		if (_context->does_something()) {
			// Read from header
			if (_size + _offset <= header_size) {
				return read_from_header(_file, reinterpret_cast<int8_t*>(_buffer), _size, _offset);
			}

			return read_encrypted_page(_page_size == 0 ? 0 : _offset / _page_size, _file, reinterpret_cast<int8_t*>(_buffer), _size, _offset);
		}

		return _base_methods->xRead(_file, _buffer, _size, _offset);
	}
	int xwrite(sqlite3_file * _file, const void * _buffer, int _size, sqlite3_int64 _offset)
	{
#if defined(PRINT_OUTPUT)
		printf("[%p]: write %i bytes to %lli\n", this, _size, _offset);
#endif

		if (_context->does_something()) {
			auto _ptr = reinterpret_cast<const int8_t*>(_buffer);

			while (_size > 0) {
				auto _result = write_encrypted_page(_page_size == 0 ? 0 : _offset / _page_size, _file, _ptr, _offset);

				if (_result != SQLITE_OK) {
					return _result;
				}

				_ptr += _page_size;
				_size -= _page_size;
				_offset += _page_size;
			}

			return SQLITE_OK;
		}

		return _base_methods->xWrite(_file, _buffer, _size, _offset);
	}
	int xfile_control(sqlite3_file * _file, int _op, void * _arg)
	{
		if (_op == SQLITE_FCNTL_PRAGMA) {
			auto _pragma = reinterpret_cast<char**>(_arg);

			if (!std::strcmp(_pragma[1], "key")) {
				printf("key: %s\n", _pragma[2]);
				_pragma[0] = sqlite3_mprintf("hi");
			} else if (!std::strcmp(_pragma[1], "mode")) {
			} else if (!std::strcmp(_pragma[1], "rekey")) {
			} else {
				return _base_methods->xFileControl(_file, _op, _arg);
			}

			return SQLITE_OK;
		}

		return _base_methods->xFileControl(_file, _op, _arg);
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
	static int xfile_control_link(sqlite3_file * _file, int _op, void * _arg)
	{
		return instance(_file)->xfile_control(_file, _op, _arg);
	}
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