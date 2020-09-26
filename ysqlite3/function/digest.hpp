#ifndef YSQLITE3_FUNCTION_DIGEST_HPP_
#define YSQLITE3_FUNCTION_DIGEST_HPP_

#include "../sqlite3.h"

namespace ysqlite3 {
namespace function {

void digest(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept;
void md5(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept;
void sha1(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept;
void sha2(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept;
void sha3(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept;

} // namespace function
} // namespace ysqlite3

#endif
