#include "transaction.hpp"

#include "database.hpp"

namespace ysqlite3 {

Transaction::Transaction(Database& db) : _db{ &db }
{
	_db->execute("BEGIN TRANSACTION;");
}

Transaction::Transaction(Transaction&& move) noexcept
{
	std::swap(_db, move._db);
}

Transaction::~Transaction() noexcept
{
	if (is_active()) {
		try {
			rollback();
		} catch (...) {
			// Fail silently instead of crashing the program.
		}
	}
}

void Transaction::rollback()
{
	if (is_active()) {
		_db->execute("ROLLBACK;");
		_db = nullptr;
	}
}

bool Transaction::is_active() const noexcept
{
	return _db != nullptr;
}

void Transaction::commit()
{
	if (is_active()) {
		_db->execute("COMMIT;");
		_db = nullptr;
	}
}

Transaction& Transaction::operator=(Transaction&& move)
{
	rollback();
	std::swap(_db, move._db);
	return *this;
}

}
