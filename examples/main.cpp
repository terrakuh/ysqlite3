#include <iostream>
#include <typeinfo>
#include <ysqlite3/database.hpp>
#include <ysqlite3/exception/base_exception.hpp>

using namespace ysqlite3;

inline int f(void*, int columns, char** values, char** names)
{
    std::cout << "got " << columns << " columns\n";

    for (auto i = 0; i < columns; ++i) {
        std::cout << names[i] << "=" << values[0] << std::endl;
    }

    return SQLITE_OK;
}

int main(int args, char** argv)
{
    try {
        database db;

        db.run("select * from test;");
        db.open("test.db");
        db.run("CREATE TABLE IF NOT EXISTS tast(noim text not null); INSERT INTO tast(noim) VALUES('heyho'), ('Musik');");
        db.run("select * from test;");
        sqlite3_exec(db.handle(), "SELECT * FROM tast;", &f, nullptr, nullptr);

    } catch (const std::exception& e) {
        std::cerr << "exception caught (" << typeid(e).name() << ")";

        if (dynamic_cast<const exception::base_exception*>(&e)) {
            auto& ex = static_cast<const exception::base_exception&>(e);

            std::cerr << " at " << ex.file() << ":" << ex.line() << " from " << ex.function();
        }

        std::cerr << "\n\t" << e.what() << std::endl;
    }

    return 0;
}