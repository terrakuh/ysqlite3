#ifndef YSQLITE3_SPAN_HPP_
#define YSQLITE3_SPAN_HPP_

#include "error.hpp"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>

namespace ysqlite3 {

constexpr std::size_t dynamic_extent = std::numeric_limits<std::size_t>::max();

template<typename Iterable>
class Span
{
public:
	Span(Iterable begin, std::size_t size) : Span{ begin, begin + size }
	{}
	Span(Iterable first, Iterable last) : _first{ std::move(first) }, _last{ std::move(last) }
	{}
	std::size_t size() const noexcept
	{
		return static_cast<std::size_t>(_last - _first);
	}
	bool empty() const noexcept
	{
		return !size();
	}
	Iterable begin()
	{
		return _first;
	}
	Iterable end()
	{
		return _last;
	}
	Span subspan(std::size_t offset, std::size_t count = dynamic_extent) const
	{
		if (offset > size()) {
			throw std::system_error{ Error::out_of_bounds };
		}
		count = std::min(count, size() - offset);
		return { _first + offset, _first + offset + count };
	}
	template<typename Type>
	Span<Type> cast() const noexcept
	{
		return { static_cast<Type>(_first), static_cast<Type>(_last) };
	}

private:
	Iterable _first;
	Iterable _last;
};

} // namespace ysqlite3

#endif
