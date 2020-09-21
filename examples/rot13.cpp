#include <iostream>
#include <typeinfo>
#include <ysqlite3/database.hpp>
#include <ysqlite3/vfs/layer/layered_vfs.hpp>
#include <ysqlite3/vfs/sqlite3_vfs_wrapper.hpp>
#include <ysqlite3/vfs/vfs.hpp>

using namespace ysqlite3;

class rot13_layer : public vfs::layer::layer
{
public:
	void encode(span<gsl::byte> buffer, sqlite3_int64) override
	{
		for (auto& i : buffer) {
			i = static_cast<gsl::byte>(static_cast<unsigned char>(i) + 13);
		}
	}
	void decode(span<gsl::byte> buffer, sqlite3_int64) override
	{
		for (auto& i : buffer) {
			i = static_cast<gsl::byte>(static_cast<unsigned char>(i) - 13);
		}
	}
};

int main(int args, char** argv)
try {
	const auto v = std::make_shared<vfs::layer::layered_vfs<vfs::sqlite3_vfs_wrapper<>, super>>(
	    vfs::find_vfs(nullptr), "rot13");
	v->add_layer<rot13_layer>();
	vfs::register_vfs(v, true);

	auto db = std::make_shared<database>();
	database db;
	db.open("test.db");
	db.execute(R"(
			
CREATE TABLE IF NOT EXISTS tast(noim text);
INSERT INTO tast(noim) VALUES('heyho'), ('Musik'), (NULL);

)");
} catch (const std::system_error& e) {
	std::cerr << "exception caught (ec=" << e.code() << "):\n\t" << e.what() << '\n';
	return -1;
}
