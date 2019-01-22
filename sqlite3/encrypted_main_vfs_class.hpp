#pragma once

#include <cstdio>
#include <memory>

#include "vfs_class.hpp"
#include "encryption_context.hpp"


namespace ysqlite3
{

class encrypted_main_vfs_class : public vfs_class
{
public:
	SQLITE3_API encrypted_main_vfs_class() noexcept;

protected:
	constexpr static auto header_size = 100;

	std::shared_ptr<encryption_context> _context;
	/** Buffer used for encryption. */
	std::vector<int8_t> _encryption_buffer;
	/** The page size. */
	uint16_t _page_size;

	SQLITE3_API void update_parameter(const int8_t * _buffer, int _size, sqlite3_int64 _offset);
	SQLITE3_API int read_from_header(sqlite3_file * _file, int8_t * _output, int _size, sqlite3_int64 _offset);
	SQLITE3_API int read_encrypted_page(encryption_context::id_t _page_id, sqlite3_file * _file, int8_t * _output, int _size, sqlite3_int64 _offset);
	SQLITE3_API int write_encrypted_page(encryption_context::id_t _page_id, sqlite3_file * _file, const int8_t * _input, sqlite3_int64 _offset);
	SQLITE3_API virtual int xopen(sqlite3_vfs * _vfs, const char * _name, sqlite3_file * _file, int _flags, int * _out_flags) override;
	SQLITE3_API virtual int xread(sqlite3_file * _file, void * _buffer, int _size, sqlite3_int64 _offset) override;
	SQLITE3_API virtual int xwrite(sqlite3_file * _file, const void * _buffer, int _size, sqlite3_int64 _offset) override;
	SQLITE3_API virtual int xfile_control(sqlite3_file * _file, int _op, void * _arg) override;
};

}