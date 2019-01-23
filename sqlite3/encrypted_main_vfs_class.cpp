#include "encrypted_main_vfs_class.hpp"
#include "address_transporter.hpp"
#include "scope_exit.hpp"


namespace ysqlite3
{

encrypted_main_vfs_class::encrypted_main_vfs_class() noexcept
{
	_page_size = 0;
}

void encrypted_main_vfs_class::update_parameter(const int8_t * _buffer, int _size, sqlite3_int64 _offset)
{
	// Page size updated
	if (_offset <= 16 && _offset + _size >= 18) {
		_page_size = read<uint16_t>(_buffer + 16 - _offset);

#if defined(PRINT_OUTPUT)
		printf("[%p]: loaded page size at %i\n", this, static_cast<int>(_page_size));
#endif
	}
}

int encrypted_main_vfs_class::read_from_header(sqlite3_file * _file, int8_t * _output, int _size, sqlite3_int64 _offset)
{
	auto _result = vfs_class::xread(_file, _output, _size, _offset);

	if (_result != SQLITE_OK) {
		// Database was newly created
		if (_result == SQLITE_IOERR_SHORT_READ) {
			_context->set_newly_created(true);
			_context->load_app_data(nullptr);
		}

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

int encrypted_main_vfs_class::read_encrypted_page(encryption_context::id_t _page_id, sqlite3_file * _file, int8_t * _output, int _size, sqlite3_int64 _offset)
{
#if defined(PRINT_OUTPUT)
	printf("[%p]: encrypted read %i bytes from %lli\n", this, _size, _offset);
#endif

	// Reserve space
	_encryption_buffer.reserve(_size);

	// Read encrypted page
	auto _result = vfs_class::xread(_file, _encryption_buffer.data(), _size, _offset);

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
		return SQLITE_CERR_DECRYPTION_FAILED;
	}

	// Overwrite app data for SQLite
	if (_page_id == 0) {
		std::memcpy(_output, "SQLite format 3", 16);
	}

	return SQLITE_OK;
}

int encrypted_main_vfs_class::write_encrypted_page(encryption_context::id_t _page_id, sqlite3_file * _file, const int8_t * _input, sqlite3_int64 _offset)
{
	// Special page because of header
	if (_page_id == 0) {
		update_parameter(_input, header_size, _offset);

		_encryption_buffer.reserve(_page_size);

		// Store app data
		_context->store_app_data(_encryption_buffer.data());
	}

#if defined(PRINT_OUTPUT)
	printf("[%p]: encrypted write %i bytes to %lli\n", this, _page_size, _offset);
#endif

	// Reserve space
	_encryption_buffer.reserve(_page_size);

	// Encrypt
	auto _size = _page_size - SQLITE3_MAX_USER_DATA_SIZE;

	std::memcpy(_encryption_buffer.data() + _size, _input + _size, SQLITE3_MAX_USER_DATA_SIZE);

	if (!_context->encrypt(_page_id, _input, _size, _encryption_buffer.data(), _encryption_buffer.data() + _size)) {
		return SQLITE_CERR_ENCRYPTION_FAILED;
	}

	// Write encrypted page to file
	return vfs_class::xwrite(_file, _encryption_buffer.data(), _size + SQLITE3_MAX_USER_DATA_SIZE, _offset);
}

int encrypted_main_vfs_class::xopen(sqlite3_vfs * _vfs, const char * _name, sqlite3_file * _file, int _flags, int * _out_flags)
{
	// Don't allow any non main db files
	if (!(_flags & SQLITE_OPEN_MAIN_DB)) {
		throw factory_error("only main dbs allowed.");
	}

	// Extract filename and context address
	std::string _path = _name;
	auto _address = address_transporter::decode_address(_path);

	// No context given
	if (!_address || _path.empty()) {
		throw factory_error("no context given.");
	}

	_context = *reinterpret_cast<const std::shared_ptr<encryption_context>*>(_address);

#if defined(PRINT_OUTPUT)
	printf("Open %s with encryption context at %p\n", _path.c_str(), _context.get());
#endif

	return vfs_class::xopen(_vfs, _path.c_str(), _file, _flags, _out_flags);
}

int encrypted_main_vfs_class::xread(sqlite3_file * _file, void * _buffer, int _size, sqlite3_int64 _offset)
{
	if (_context->does_something()) {
		// Read from header
		if (_size + _offset <= header_size) {
			return read_from_header(_file, reinterpret_cast<int8_t*>(_buffer), _size, _offset);
		}

		return read_encrypted_page(_page_size == 0 ? 0 : _offset / _page_size, _file, reinterpret_cast<int8_t*>(_buffer), _size, _offset);
	}

	return vfs_class::xread(_file, _buffer, _size, _offset);
}

int encrypted_main_vfs_class::xwrite(sqlite3_file * _file, const void * _buffer, int _size, sqlite3_int64 _offset)
{
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

	return vfs_class::xwrite(_file, _buffer, _size, _offset);
}

int encrypted_main_vfs_class::xfile_control(sqlite3_file * _file, int _op, void * _arg)
{
	if (_op == SQLITE_FCNTL_PRAGMA) {
		auto _pragma = reinterpret_cast<char**>(_arg);
		const auto _try_unlock = [&]() {
			auto _decrypted = true;

			if (_page_size) {
				_encryption_buffer.reserve(_page_size);

				_decrypted = xread(_file, _encryption_buffer.data(), _page_size, 0) != SQLITE_CERR_DECRYPTION_FAILED;
			}

			return _decrypted;
		};

		if (!std::strcmp(_pragma[1], "key")) {
			if (_pragma[2]) {
				try {
					key_t _key = base64::decode(_pragma[2], std::char_traits<char>::length(_pragma[2]));

					scope_exit _cleaner([&]() {
						_key.assign(_key.length(), 'x');

						auto _ptr = _pragma[2];

						while (*_ptr) {
							*_ptr++ = 'x';
						}
					});

					_context->set_key(_key, true);
					_context->set_key(_key, false);
				} catch (...) {
					goto gt_key_failed;
				}

				if (_try_unlock()) {
					_pragma[0] = sqlite3_mprintf("ok.");
					_context->apply_key();
				} else {
				gt_key_failed:;
					_pragma[0] = sqlite3_mprintf("failed.");
					_context->revert_key();
				}
			} else {
				_pragma[0] = sqlite3_mprintf("key required.");
			}
		} else if (!std::strcmp(_pragma[1], "try_unlock")) {
			_pragma[0] = sqlite3_mprintf(_try_unlock() ? "ok." : "failed.");
		} else if (!std::strcmp(_pragma[1], "page_size") && _pragma[2]) {
			// Invalid page size
			if (std::atoi(_pragma[2]) < 1024) {
				_pragma[0] = sqlite3_mprintf("invalid size for an encrypted database. minimum page is 1024.");
			} else {
				goto gt_pass;
			}
		} else {
			goto gt_pass;
		}

		return SQLITE_OK;
	}

gt_pass:;

	return vfs_class::xfile_control(_file, _op, _arg);
}

}