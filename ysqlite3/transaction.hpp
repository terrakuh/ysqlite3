#ifndef YSQLITE3_TRANSACTION_HPP_
#define YSQLITE3_TRANSACTION_HPP_

#include <memory>

namespace ysqlite3 {

class Database;

class Transaction
{
public:
	Transaction(std::shared_ptr<Database> db) noexcept;
	/// The moved object will be invalid after the move operation.
	Transaction(Transaction&& move) noexcept;
	/// Rolls the transaction back if it was not committed.
	~Transaction() noexcept;
	/**
	 * Commits the transaction.
	 *
	 * @post the transaction is not open
	 *
	 * @exception see database::execute()
	 */
	void commit();
	/**
	 * Rollbacks the transaction.
	 *
	 * @post the transaction is not open
	 *
	 * @exception see database::execute()
	 */
	void rollback();
	bool is_active() const noexcept;
	/// Calls is_active().
	operator bool() const noexcept;
	Transaction& operator=(Transaction&& move) noexcept;

private:
	friend Database;

	std::shared_ptr<Database> _db;
};

} // namespace ysqlite3

#endif
