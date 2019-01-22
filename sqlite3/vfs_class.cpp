#include "vfs_class.hpp"


namespace ysqlite3
{

vfs_class::xopen_signature_t vfs_class::_xopen_base = nullptr;
sqlite3_vfs vfs_class::_vfs;
size_t vfs_class::_max_class_size = sizeof(vfs_class);
std::vector<vfs_class::factory_t> vfs_class::_factories({ [](void * _buffer) { new(_buffer) vfs_class(); } });

vfs_class::vfs_class() noexcept : _derived_methods{}
{
#if defined(PRINT_OUTPUT)
	printf("[%p]: creating\n", this);
#endif

	_base_methods = nullptr;
}

vfs_class::~vfs_class() noexcept
{
#if defined(PRINT_OUTPUT)
	printf("[%p]: destroying\n", this);
#endif
}

void vfs_class::register_vfs()
{
	sqlite3_initialize();

	// Get default VFS
	auto _default = sqlite3_vfs_find(nullptr);

	_xopen_base = _default->xOpen;

	std::memcpy(&_vfs, _default, sizeof(_vfs));

	_vfs.szOsFile += _max_class_size;
	_vfs.pNext = nullptr;
	_vfs.zName = SQLITE3_CUSTOM_VFS_NAME;
	_vfs.xOpen = &vfs_class::xopen_link;

	sqlite3_vfs_register(&_vfs, 0);
}

int vfs_class::xopen_link(sqlite3_vfs * _vfs, const char * _name, sqlite3_file * _file, int _flags, int * _out_flags)
{
	const auto _ptr = reinterpret_cast<vfs_class*>(reinterpret_cast<int8_t*>(_file) + vfs_class::_vfs.szOsFile - _max_class_size);

	for (auto & _factory : _factories) {
		try {
			_factory(_ptr);

			return _ptr->xopen(_vfs, _name, _file, _flags, _out_flags);
		} catch (...) {

		}
	}

	throw;
}

bool vfs_class::little_endian() noexcept
{
	int _test = 1;

	return *reinterpret_cast<char*>(&_test);
}

int vfs_class::xopen(sqlite3_vfs * _vfs, const char * _name, sqlite3_file * _file, int _flags, int * _out_flags)
{
	auto _result = _xopen_base(_vfs, _name, _file, _flags, _out_flags);

	// Failed to open
	if (_result != SQLITE_OK) {
		// Delete this
		if (!_file->pMethods) {
			this->~vfs_class();
		}

		return _result;
	}

	_base_methods = _file->pMethods;

	// Override base methods
	_derived_methods = sqlite3_io_methods{ *_file->pMethods };

	_derived_methods.xClose = &vfs_class::xclose_link;
	_derived_methods.xSync = &vfs_class::xsync_link;
	_derived_methods.xRead = &vfs_class::xread_link;
	_derived_methods.xWrite = &vfs_class::xwrite_link;
	_derived_methods.xFileControl = &vfs_class::xfile_control_link;

	_file->pMethods = &_derived_methods;

	return SQLITE_OK;
}

int vfs_class::xclose(sqlite3_file * _file)
{
#if defined(PRINT_OUTPUT)
	printf("[%p]: closing\n", this);
#endif

	_file->pMethods = _base_methods;

	this->~vfs_class();

	return _file->pMethods->xClose(_file);
}

int vfs_class::xsync(sqlite3_file * _file, int _flags)
{
#if defined(PRINT_OUTPUT)
	printf("[%p]: syncing\n", this);
#endif

	return _base_methods->xSync(_file, _flags);
}

int vfs_class::xread(sqlite3_file * _file, void * _buffer, int _size, sqlite3_int64 _offset)
{
#if defined(PRINT_OUTPUT)
	printf("[%p]: read %i bytes from %lli\n", this, _size, _offset);
#endif

	return _base_methods->xRead(_file, _buffer, _size, _offset);
}

int vfs_class::xwrite(sqlite3_file * _file, const void * _buffer, int _size, sqlite3_int64 _offset)
{
#if defined(PRINT_OUTPUT)
	printf("[%p]: write %i bytes to %lli\n", this, _size, _offset);
#endif

	return _base_methods->xWrite(_file, _buffer, _size, _offset);
}

int vfs_class::xfile_control(sqlite3_file * _file, int _op, void * _arg)
{
	return _base_methods->xFileControl(_file, _op, _arg);
}

int vfs_class::xclose_link(sqlite3_file * _file)
{
	return instance(_file)->xclose(_file);
}

int vfs_class::xsync_link(sqlite3_file * _file, int _flags)
{
	return instance(_file)->xsync(_file, _flags);
}

int vfs_class::xread_link(sqlite3_file * _file, void * _buffer, int _size, sqlite3_int64 _offset)
{
	return instance(_file)->xread(_file, _buffer, _size, _offset);
}

int vfs_class::xwrite_link(sqlite3_file * _file, const void * _buffer, int _size, sqlite3_int64 _offset)
{
	return instance(_file)->xwrite(_file, _buffer, _size, _offset);
}

int vfs_class::xfile_control_link(sqlite3_file * _file, int _op, void * _arg)
{
	return instance(_file)->xfile_control(_file, _op, _arg);
}

vfs_class * vfs_class::instance(sqlite3_file * _file)
{
	return reinterpret_cast<vfs_class*>(reinterpret_cast<int8_t*>(_file) + _vfs.szOsFile - _max_class_size);
}

}