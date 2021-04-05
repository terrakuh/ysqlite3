#include <iostream>
#include <ysqlite3/database.hpp>
#include <ysqlite3/finally.hpp>

using namespace ysqlite3;

class Match : public function::Function
{
public:
	Match() : Function{ 1, true, true, function::text_encoding::utf8 }
	{}

protected:
	void run(sqlite3_context* context, int argc, sqlite3_value** argv) override
	{
		puts("matching");
		sqlite3_result_int(context, 1);
	}
};

class Exec : public function::Function
{
public:
	Exec() : Function{ -1, true, true, function::text_encoding::utf8 }
	{}

protected:
	void run(sqlite3_context* context, int argc, sqlite3_value** argv) override
	{
		if (argc < 1) {
			sqlite3_result_error(context, "requires at least one argument", -1);
			return;
		}
		const auto db      = sqlite3_context_db_handle(context);
		const auto sql     = reinterpret_cast<const char*>(sqlite3_value_text(argv[0]));
		sqlite3_stmt* stmt = nullptr;
		int ec             = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
		if (ec != SQLITE_OK) {
			sqlite3_result_error(context, sqlite3_errmsg(db), -1);
			return;
		}
		const auto _ = finally([&] { sqlite3_finalize(stmt); });
		for (int i = 1; i < argc; ++i) {
			sqlite3_bind_value(stmt, i, argv[i]);
		}
		// execute
		while (true) {
			ec = sqlite3_step(stmt);
			if (ec != SQLITE_ROW) {
				sqlite3_reset(stmt);
				if (ec == SQLITE_DONE) {
					sqlite3_result_null(context);
					return;
				}
				sqlite3_result_error(context, sqlite3_errmsg(db), -1);
				return;
			}
		}
	}
};

int main(int args, char** argv)
try {
	Database db;
	db.open("/tmp/test/sybil.db");
	db.register_function("exec", std::unique_ptr<function::Function>{ new Exec{} });
	db.register_function("foo", std::unique_ptr<function::Function>{ new Match{} });
	db.execute(R"(update media set name="lol, wie geil" where id=1)");
	// db.execute(R"del(

	// 	select exec("select foo(2)")

	// )del");
	// return 0;
	// 	db.execute(R"(
	// 		create view media_view as
	// 			select media.id as id, media.name as name, media.extension as extension,
	// group_concat(tag.name,
	// "~") as tag, group_concat(person_name.name, "~") as person 			from media 			join
	// media_person on media_person.media=media.id 			join person_name on
	// person_name.person=media_person.person 			join media_tag
	// on media_tag.media=media.id 			join tag on tag.id=media_tag.tag 			group by media.id;
	// drop table search; 		create virtual table if not exists search using fts5(name, extension, tag,
	// person, content=media_view, content_rowid=id); 		insert into search(rowid, name, extension, tag,
	// person) select id, name, extension, tag, person from media_view;
	// )");
	// 	db.execute(R"(
	// -- ALTER TABLE media
	// -- ADD COLUMN grouped_tags TEXT;
	// --
	// -- ALTER TABLE media
	// -- ADD COLUMN grouped_people TEXT;

	// UPDATE media
	// SET grouped_tags=group_concat(tag.name, "~")
	// FROM media_tag
	// JOIN tag ON tag.id=media_tag.tag AND media_tag.media=media.id;

	// UPDATE media
	// SET grouped_people=group_concat(person_name.name, "~")
	// FROM person_name
	// JOIN media_person ON media_person.media=media.id AND media_person.person=person_name.person;

	// 	)");
	// 	db.execute(R"(

	// DROP TABLE media_fts;
	// CREATE VIRTUAL TABLE media_fts USING fts5(name, extension, person, tag, content="");
	// INSERT INTO media_fts(rowid, name, extension, person, tag)
	// 	SELECT media.id, media.name, media.extension, person.people, tag.tags
	// 	FROM media
	// 	LEFT JOIN (
	// 		SELECT media_person.media AS media, GROUP_CONCAT(person_name.name, "~~") AS people
	// 		FROM person_name
	// 		JOIN media_person ON media_person.person=person_name.person
	// 		GROUP BY media_person.media
	// 	) person ON person.media=media.id
	// 	LEFT JOIN (
	// 		SELECT media_tag.media AS media, GROUP_CONCAT(tag.name, "~~") AS tags
	// 		FROM tag
	// 		JOIN media_tag ON media_tag.tag=tag.id
	// 		GROUP BY media_tag.media
	// 	) tag ON tag.media=media.id

	// 	)");

	// auto stmt = db.prepare_statement("select name from search where search match ?");
	auto stmt = db.prepare_statement(
	    R"(select name, exec("select id from media limit 1") from media where id in (select rowid from media_fts where media_fts match ?))");
	Results r;
	std::string s;
	std::cout << "query> ";
	std::getline(std::cin, s);
	stmt.bind(0, s);

	while ((r = stmt.step())) {
		std::cout << r.text(0) << "," << r.integer(1) << std::endl;
	}
} catch (const std::system_error& e) {
	std::cerr << "exception caught (ec=" << e.code() << "):\n\t" << e.what() << '\n';
	return -1;
}
