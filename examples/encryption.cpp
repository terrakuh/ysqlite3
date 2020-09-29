#include <array>
#include <cstring>
#include <iostream>
#include <typeinfo>
#include <ysqlite3/database.hpp>
#include <ysqlite3/vfs/crypt_file.hpp>
#include <ysqlite3/vfs/sqlite3_file_wrapper.hpp>
#include <ysqlite3/vfs/sqlite3_vfs_wrapper.hpp>
#include <ysqlite3/vfs/vfs.hpp>

using namespace ysqlite3;

int main(int args, char** argv)
try {
	vfs::register_vfs(std::make_shared<vfs::sqlite3_vfs_wrapper<vfs::crypt_file<vfs::sqlite3_file_wrapper>>>(
	                      vfs::find_vfs(nullptr), "aes-gcm"),
	                  true);

	database db;
	std::remove("test.db");
	std::remove("test.db-journal");
	std::remove("test.db-wal");
	db.open("test.db", open_flag_readwrite | open_flag_create, "aes-gcm");
	db.set_reserved_size(vfs::crypt_file_reserve_size());
	// db.set_journal_mode(journal_mode::memory);
	db.execute(R"(

PRAGMA plain_key='this is my key';

CREATE TABLE IF NOT EXISTS tast(noim text);
INSERT INTO tast(noim) VALUES('heyho');

)");
	db.execute("BEGIN TRANSACTION");
	for (auto i = 0; i < 300; ++i) {
		db.execute("INSERT INTO tast(noim) VALUES('heyho');");
	}
	db.execute("COMMIT");

	auto stmt = db.prepare_statement("SELECT * FROM tast");
	results r;

	while ((r = stmt.step())) {
		for (const auto& column : stmt.columns()) {
			const auto text = r.text(column.c_str());
			std::cout << column << ": " << (!text ? "<null>" : text) << '\n';
		}
	}
} catch (const std::system_error& e) {
	std::cerr << "exception caught (ec=" << e.code() << "):\n\t" << e.what() << '\n';
	return -1;
}
