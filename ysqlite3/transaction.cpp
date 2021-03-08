#include "transaction.hpp"

#include "database.hpp"

using namespace ysqlite3;

Transaction::Transaction(std::shared_ptr<Database> db) noexcept : _db{ std::move(db) }
{
	_db->execute("BEGIN TRANSACTION;");
}

Transaction::Transaction(Transaction&& move) noexcept
{
	_db      = move._db;
	move._db = nullptr;
}

Transaction::~Transaction() noexcept
{
	if (_db) {
		rollback();
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
	return static_cast<bool>(_db);
}

Transaction::operator bool() const noexcept
{
	return is_active();
}

void Transaction::commit()
{
	if (is_active()) {
		_db->execute("COMMIT;");
		_db = nullptr;
	}
}

Transaction& Transaction::operator=(Transaction&& move) noexcept
{
	rollback();
	std::swap(_db, move._db);
	return *this;
}
