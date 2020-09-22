#ifndef YSQLITE3_SPAN_HPP_
#define YSQLITE3_SPAN_HPP_

#include <cstddef>
#include <utility>

namespace ysqlite3 {

template<typename Iterator>
class span
{
public:
	span(Iterator begin, std::size_t size) : span{ begin, begin + size }
	{}
	span(Iterator first, Iterator last) : _first{ std::move(first) }, _last{ std::move(last) }
	{}
	std::size_t size() const noexcept
	{
		return static_cast<std::size_t>(_last - _first);
	}
	bool empty() const noexcept
	{
		return !size();
	}
	Iterator begin()
	{
		return _first;
	}
	Iterator end()
	{
		return _last;
	}

private:
	Iterator _first;
	Iterator _last;
};

} // namespace ysqlite3

#endif
