#include <iostream>
#include <typeinfo>
#include <ysqlite3/database.hpp>
#include <ysqlite3/vfs/page_transforming_file.hpp>
#include <ysqlite3/vfs/sqlite3_file_wrapper.hpp>
#include <ysqlite3/vfs/sqlite3_vfs_wrapper.hpp>
#include <ysqlite3/vfs/vfs.hpp>

using namespace ysqlite3;

class rot13_file : public vfs::page_transforming_file<vfs::sqlite3_file_wrapper>
{
public:
	using parent = vfs::page_transforming_file<vfs::sqlite3_file_wrapper>;
	using parent::parent;

protected:
	void encode_page(span<std::uint8_t*> page) override
	{
		for (auto& i : page) {
			i += 13;
		}
	}
	void decode_page(span<std::uint8_t*> page) override
	{
		for (auto& i : page) {
			i -= 13;
		}
	}
};

int main(int args, char** argv)
try {
	vfs::register_vfs(std::make_shared<vfs::sqlite3_vfs_wrapper<rot13_file>>(vfs::find_vfs(nullptr), "rot13"),
	                  false);

	database db;
	db.open("test.db", open_flag_readwrite | open_flag_create, "rot13");
	db.execute(R"(

CREATE TABLE IF NOT EXISTS tast(noim text);
INSERT INTO tast(noim) VALUES('heyho'), ('Musik'), (NULL);

)");

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
