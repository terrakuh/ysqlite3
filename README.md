# ysqlite3

A handy SQLite3 wrapper with additional VFS for encryption

## Building

### Compiling & Installing

```sh
git clone https://github.com/terrakuh/ysqlite3.git
cd ysqlite3

cmake . && cmake --build .

cmake --build . --target install
```

### CMake

```cmake
find_package(ysqlite3 REQUIRED)

add_executable(main "main.cpp")
target_link_libraries(main ysqlite3::ysqlite3)
```

## Usage

### Simple example

```cpp
#include <ysqlite3/database.hpp>

using namespace ysqlite3;

int main()
{
	database db{"my.db"};

	db.execute(R"(

CREATE TABLE IF NOT EXISTS t(
	name text
);

INSERT INTO t(name) VALUES('hello'), ('world'), (NULL);

	)");

	auto stmt = db.prepare_statement("SELECT * FROM t WHERE name!=?;");
	results r;
	stmt.bind(1, nullptr);

	while ((r = stmt.step())) {
		for (const auto& column : stmt.columns()) {
			const auto text = r.text(column.c_str());
			std::cout << column << ": " << (!text ? "<null>" : text) << '\n';
		}
	}
}
```

### Custom VFS layers

```cpp
#include <ysqlite3/database.hpp>
#include <ysqlite3/vfs/page_transforming_file.hpp>
#include <ysqlite3/vfs/sqlite3_file_wrapper.hpp>
#include <ysqlite3/vfs/sqlite3_vfs_wrapper.hpp>

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

int main()
{
	vfs::register_vfs(std::make_shared<vfs::sqlite3_vfs_wrapper<rot13_file>>(vfs::find_vfs(nullptr), "rot13"),
	                  false);

	database db;
	db.open("test.db", open_flag_readwrite | open_flag_create, "rot13");
}
```

## License

[MIT](https://github.com/terrakuh/ysqlite3/blob/master/LICENSE) License.
