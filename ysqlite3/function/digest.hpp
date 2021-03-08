#ifndef YSQLITE3_FUNCTION_DIGEST_HPP_
#define YSQLITE3_FUNCTION_DIGEST_HPP_

#include "../sqlite3.h"

namespace ysqlite3 {
namespace function {

/// In SQLite: DIGEST(<digset_name>, ...). Example: DIGEST("md5", "some test")
void digest(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept;
/// Alias for DIGEST("md5", ...)
void md5(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept;
/// Alias for DIGEST("sha1", ...)
void sha1(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept;
/// In SQLite: SHA2(<size>, ...). Example: SHA2(256, "some test")
void sha2(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept;
/// In SQLite: SHA3(<size>, ...). Example: SHA2(256, "some test")
void sha3(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept;

} // namespace function
} // namespace ysqlite3

#endif
