# ysqlite3
A handy SQLite3 wrapper with additional VFS for encryption or other transforming layers.

## Building
### Compiling and Installing
```sh
git clone --recurse-submodules https://github.com/terrakuh/ysqlite3.git && cd ysqlite3
mkdir build && cd build

cmake ..
cmake --build .

ctest

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
  Database db{"my.db"};
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
Here is a simple [ROT13](https://en.wikipedia.org/wiki/ROT13) encryption file layer:

```cpp
#include <iostream>
#include <typeinfo>
#include <ysqlite3/database.hpp>
#include <ysqlite3/vfs/page_transforming_file.hpp>
#include <ysqlite3/vfs/sqlite3_file_wrapper.hpp>
#include <ysqlite3/vfs/sqlite3_vfs_wrapper.hpp>
#include <ysqlite3/vfs/vfs.hpp>

using namespace ysqlite3;

class Rot13_file : public vfs::Page_transforming_file<vfs::SQLite3_file_wrapper>
{
public:
  using Parent = vfs::Page_transforming_file<vfs::SQLite3_file_wrapper>;
  using Parent::Parent;

protected:
  void encode_page(Span<std::uint8_t*> page) override
  {
    for (auto& i : page) {
      i += 13;
    }
  }
  void decode_page(Span<std::uint8_t*> page) override
  {
    for (auto& i : page) {
      i -= 13;
    }
  }
};

int main()
{
  vfs::register_vfs(std::make_shared<vfs::SQLite3_vfs_wrapper<Rot13_file>>(vfs::find_vfs(nullptr), "rot13"),
                    false);

  Database db;
  db.open("test.db", open_flag_readwrite | open_flag_create, "rot13");
  // use db
}
```

### As Extension
```c
sqlite3 db;
// register vfs for the 'real' database
sqlite3_open(":memory:", &db);
sqlite3_enable_load_extension(db, 1);
sqlite3_load_extension(db, "/path/to/ysqlite3_crypt_vfs.ext", "ysqlite3_register_crypt_vfs", 0);
sqlite3_close(db);

sqlite3_open_v2("file:real.db?key=x'0f23eabc39'&cipher=aes-256-gcm", &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_URI, "ysqlite3-crypt");
// automatically initializes the database
sqlite3_enable_load_extension(db, 1);
sqlite3_load_extension(db, "/path/to/ysqlite3_crypt_vfs.ext", "ysqlite3_init_crypt_db", 0);
sqlite3_enable_load_extension(db, 0);
// call the following if you don't want to use the method above
// note: if the db already exists you may need to call 'VACUUM'
int n = 32;
sqlite3_file_control(db, 0, SQLITE_FCNTL_RESERVE_BYTES, &n);
```

## Shell
```sh
ysqlite3 -vfs ysqlite3-crypt encrypted.db
sqlite> .filectrl reserve_bytes 32 #only required if first created
sqlite> PRAGMA cipher="aes-256-gcm";
sqlite> PRAGMA key="r'password123'";
```

## License

[MIT](https://github.com/terrakuh/ysqlite3/blob/master/LICENSE) License.
