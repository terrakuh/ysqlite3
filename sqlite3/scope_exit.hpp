#pragma once

#include <functional>


namespace ysqlite3
{

/**
 * @brief A simple scope guard.
*/
class scope_exit
{
public:
	/** The exit function type. */
	typedef std::function<void()> function_t;

	/**
	 * Constructor.
	 *
	 * @param [in] _exit The exit function.
	*/
	scope_exit(function_t && _exit) noexcept;
	/**
	 * Destructor.
	 *
	 * @throws See @a _exit.
	*/
	~scope_exit();
	void cancel() noexcept;

private:
	/** The function that is called on destruction. */
	function_t _exit;
};

}