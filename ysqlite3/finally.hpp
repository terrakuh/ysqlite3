#ifndef YSQLITE3_FINALLY_HPP_
#define YSQLITE3_FINALLY_HPP_

#include <type_traits>
#include <utility>

namespace ysqlite3 {

/// Executed when this object dies. The action may not throw.
template<typename Action>
class final_action
{
public:
	final_action(Action&& action) : _action{ std::move(action) }
	{}
	final_action(const Action& action) : _action{ action }
	{}
	final_action(final_action&& move) : _action{ std::move(move._action) }
	{
		_valid      = move._valid;
		move._valid = false;
	}
	~final_action() noexcept
	{
		_action();
	}
	final_action& operator=(final_action&& move)
	{
		if (_valid) {
			_action();
		}
		_action     = std::move(move._action);
		_valid      = move._valid;
		move._valid = false;
		return *this;
	}

private:
	Action _action;
	bool _valid = true;
};

/// Creates a final action.
template<typename Action>
inline final_action<typename std::decay<Action>::type> finally(Action&& action)
{
	return { std::forward<Action>(action) };
}

} // namespace ysqlite3

#endif
