# ysqlite3

A handy SQLite3 wrapper with additional VFS for encryption

## Building

```sh
git clone https://github.com/terrakuh/ysqlite3.git
cd ysqlite3

cmake . && cmake --build .

cmake --build . --target install
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

auto stmt = db.prepare_statement(R"(
SELECT name FROM t WHERE name!=?;
)");
ysqlite3::results results;

stmt.bind(1, nullptr);

while ((results = stmt.step())) {
	std::cout << "Name: " << results.text("name") << std::endl;
}
```

## License

The wrapper is put into the public domain.
