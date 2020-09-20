#ifndef YSQLITE3_TRANSACTION_HPP_
#define YSQLITE3_TRANSACTION_HPP_

#include <memory>

namespace ysqlite3 {

class database;

class transaction
{
public:
	transaction(const transaction& copy) = delete;
	/**
	 * Move-Constructor.
	 *
	 * @post move is invalid
	 *
	 * @param[in,out] move the transaction that should be moved
	 */
	transaction(transaction&& move) noexcept;
	/**
	 * Destructor. If the transaction was neither committed nor rolldd back, it will be rolled back.
	 */
	~transaction();
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
	/**
	 * Tests whether this transaction is open.
	 *
	 * @return `true` if it is open, otherwise `false`
	 */
	bool is_open() const noexcept;
	/**
	 * Tests whether this transaction is open.
	 *
	 * @return `true` if it is open, otherwise `false`
	 */
	operator bool() const noexcept;
	transaction& operator=(transaction&& move) noexcept;

private:
	friend database;

	std::shared_ptr<database> _db;

	transaction(std::shared_ptr<database> db) noexcept;
};

} // namespace ysqlite3

#endif
