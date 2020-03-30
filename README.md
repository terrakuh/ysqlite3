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
ysqlite3::database db("my.db");

db.execute(R"(

CREATE TABLE IF NOT EXISTS t(
	name text
);

INSERT INTO t(name) VALUES('hello'), ('world'), (NULL);

)");

auto stmt = db.prepare_statement("SELECT name FROM t WHERE name!=?;");
ysqlite3::results results;

stmt.bind(1, nullptr);

while ((results = stmt.step())) {
	std::cout << "Name: " << results.text("name") << std::endl;
}
```

### Custom VFS layers

```cpp
using namespace ysqlite3;

class rot13_layer : public vfs::layer::layer
{
public:
	void encode(gsl::span<gsl::byte> buffer, sqlite3_int64) override
	{
		for (auto& i : buffer) {
			i = static_cast<gsl::byte>(static_cast<unsigned char>(i) + 13);
		}
	}
	void decode(gsl::span<gsl::byte> buffer, sqlite3_int64) override
	{
		for (auto& i : buffer) {
			i = static_cast<gsl::byte>(static_cast<unsigned char>(i) - 13);
		}
	}
};

// adds a Rot13 encoder on top of the default VFS
auto v = std::make_shared<vfs::layer::layered_vfs<vfs::sqlite3_vfs_wrapper<>,
                          vfs::layer::layered_file>>(
			  vfs::find_vfs(nullptr), "myvfs");

v->add_layer<rot13_layer>();

// register and don't make it the new default
vfs::register_vfs(v, false);

// open database with custom VFS
db.open("my.db", database::open_flag_readwrite | database::open_flag_create, "myvfs");
```

## License

The wrapper is put into the public domain.
