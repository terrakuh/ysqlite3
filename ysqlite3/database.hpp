#pragma once

#include "sqlite3.h"

#include <iostream>
#include <string>

namespace ysqlite3 {

class statement;
class result;

class database
{
public:
    database(const std::string& file);
    database(const std::u16string& file);
    database(std::iostream inout);


    statement create_statement(const std::string& query);
    statement create_statement(const std::u16string& query);
    result execute(const std::string& query);
    result execute(const std::u16string& query);

    result operator<<(const std::string& query);
    result operator<<(const std::u16string& query);

private:
    sqlite3* _database;
};

} // namespace ysqlite3