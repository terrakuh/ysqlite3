#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>
#include <cstring>
#include <random>
#include <string>
#include <vector>
#include <ysqlite3/database.hpp>
#include <ysqlite3/vfs/crypt_file.hpp>
#include <ysqlite3/vfs/sqlite3_file_wrapper.hpp>
#include <ysqlite3/vfs/sqlite3_vfs_wrapper.hpp>

using namespace ysqlite3;

TEST_CASE("opening crypt db")
{
	vfs::register_vfs(std::make_shared<vfs::SQLite3_vfs_wrapper<vfs::Crypt_file<vfs::SQLite3_file_wrapper>>>(
	                      vfs::find_vfs(nullptr), YSQLITE3_CRYPT_VFS_NAME),
	                  true);

	REQUIRE(vfs::find_vfs(YSQLITE3_CRYPT_VFS_NAME) == vfs::find_vfs(nullptr));
	REQUIRE(vfs::find_vfs(nullptr));

	std::vector<std::uint8_t> data;

	{
		std::remove("test.db");
		Database db{ "test.db" };
		db.set_reserved_size(vfs::crypt_file_reserve_size());
		db.execute(R"(

	PRAGMA key="r'my secret'";
	PRAGMA cipher="aes-256-gcm";
	CREATE TABLE crypt(val BLOB);

		)");

		// generate payload
		data.reserve(4096 * 2);
		std::default_random_engine engine{ 616161 };
		std::uniform_int_distribution<std::uint8_t> distribution{ std::numeric_limits<std::uint8_t>::lowest(),
			                                                      std::numeric_limits<std::uint8_t>::max() };

		for (auto& i : data) {
			i = distribution(engine);
		}

		auto stmt = db.prepare_statement("INSERT INTO crypt(val) VALUES(?)");
		stmt.bind_reference(0, { data.data(), data.size() });
		stmt.finish();
	}

	{
		Database db;
		db.open("test.db", open_flag_readwrite);
		db.execute(R"(

	PRAGMA key="r'my secret'";
	PRAGMA cipher="aes-256-gcm";

		)");

		// compare data
		auto stmt = db.prepare_statement("SELECT val FROM crypt");
		auto r    = stmt.step();
		REQUIRE(r);

		auto blob = r.blob(0);
		REQUIRE(blob.size() == data.size());
		REQUIRE(std::memcmp(blob.begin(), data.data(), data.size()) == 0);
	}

	{
		Database db;
		db.open("file:test.db?key=r%27my%20secret%27&cipher=aes-256-gcm",
		        open_flag_readwrite | open_flag_uri);

		// compare data
		auto stmt = db.prepare_statement("SELECT val FROM crypt");
		auto r    = stmt.step();
		REQUIRE(r);

		auto blob = r.blob(0);
		REQUIRE(blob.size() == data.size());
		REQUIRE(std::memcmp(blob.begin(), data.data(), data.size()) == 0);
	}
}
