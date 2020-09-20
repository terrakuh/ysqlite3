#include "transaction.hpp"

#include "database.hpp"

using namespace ysqlite3;

transaction::transaction(transaction&& move) noexcept
{
	_db      = move._db;
	move._db = nullptr;
}

transaction::~transaction()
{
	if (_db) {
		rollback();
	}
}

void transaction::rollback()
{
	if (is_open()) {
		_db->execute("ROLLBACK;");
		_db = nullptr;
	}
}

bool transaction::is_open() const noexcept
{
	return _db;
}

transaction::operator bool() const noexcept
{
	return is_open();
}

transaction::transaction(std::shared_ptr<database> db) noexcept : _db{ std::move(db) }
{}

void transaction::commit()
{
	if (is_open()) {
		_db->execute("COMMIT;");
		_db = nullptr;
	}
}

transaction& transaction::operator=(transaction&& move) noexcept
{
	rollback();
	std::swap(_db, move._db);
	return *this;
}
