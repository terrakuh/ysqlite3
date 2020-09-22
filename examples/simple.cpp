#include <iostream>
#include <ysqlite3/database.hpp>

using namespace ysqlite3;

int main(int args, char** argv)
try {
	database db;
	db.open("test.db");
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
	std::cerr << "exception caught (ec" << e.code() << "):\n\t" << e.what() << '\n';
	return -1;
}
