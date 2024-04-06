#pragma once

namespace ysqlite3 {

class Database;

class Transaction
{
public:
	/// Automatically begins a transaction. The database must be valid until this transaction completes i.e.
	/// `is_active() == false`.
	explicit Transaction(Database& db);
	Transaction(const Transaction& copy) = delete;
	Transaction(Transaction&& move) noexcept;
	/// Rolls the transaction back if it was not committed.
	~Transaction() noexcept;

	/**
	 * Commits the transaction. If the transaction is not active, nothing happens.
	 *
	 * @post `is_active() == false`
	 *
	 * @exception see database::execute()
	 */
	void commit();
	/**
	 * Rollbacks the transaction. If the transaction is not active, nothing happens.
	 *
	 * @post `is_active() == false`
	 *
	 * @exception see database::execute()
	 */
	void rollback();
	[[nodiscard]] bool is_active() const noexcept;

	Transaction& operator=(const Transaction& copy) = delete;
	Transaction& operator=(Transaction&& move);

private:
	Database* _db = nullptr;
};

} // namespace ysqlite3
