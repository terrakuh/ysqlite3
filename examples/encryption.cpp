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

class test : public vfs::page_transforming_file<vfs::sqlite3_file_wrapper>
{
public:
	using page_transforming_file<vfs::sqlite3_file_wrapper>::page_transforming_file;

protected:
	void encode_page(span<std::uint8_t*> page) override
	{}
	void decode_page(span<std::uint8_t*> page) override
	{}
};

int main(int args, char** argv)
try {
	// register encryption VFS
	vfs::register_vfs(std::make_shared<vfs::sqlite3_vfs_wrapper<vfs::crypt_file<vfs::sqlite3_file_wrapper>>>(
	                      vfs::find_vfs(nullptr), "aes-gcm"),
	                  true);

	database db;
	// std::remove("test.db");
	// std::remove("test.db-journal");
	// std::remove("test.db-wal");
	db.open("test.db", open_flag_readwrite | open_flag_create);

	// this is REQUIRED when the database is first opened, before any other action!
	db.set_reserved_size(vfs::crypt_file_reserve_size());

	switch (2) {
	case 0:
		db.execute(R"(

PRAGMA crypt_transformation=1;
PRAGMA plain_key="password123";
PRAGMA cipher="aes-256-gcm";
VACUUM;
PRAGMA crypt_transformation=0;

INSERT INTO tm(v) VALUES("my third insert");

	)");
		break;
	case 1:
		db.execute(R"(

CREATE TABLE tm(v text);
INSERT INTO tm(v) VALUES("my first insert"), ("another insert");

	)");
		break;
	case 2: {
		db.execute(R"(

PRAGMA key="x'070617373776f7264313233'";
PRAGMA cipher="aes-256-gcm";

		)");
		auto stmt = db.prepare_statement("SELECT * FROM tm");
		results r;

		while ((r = stmt.step())) {
			for (const auto& column : stmt.columns()) {
				const auto text = r.text(column.c_str());
				std::cout << column << ": " << (!text ? "<null>" : text) << '\n';
			}
		}
		break;
	}
	}

	return 0;

	db.execute("PRAGMA page_size=2048; VACUUM");
	db.execute("INSERT INTO tast(noim) VALUES('heyho');");
	return 0;

	db.execute(R"(

PRAGMA plain_key='this is my old key';
PRAGMA crypt_transformation=1;
PRAGMA plain_key='this is my new key';
VACUUM;
PRAGMA crypt_transformation=0;

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
