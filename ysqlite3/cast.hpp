#pragma once

#include "error.hpp"

#include <type_traits>
#include <utility>

namespace ysqlite3 {

template<typename To, typename From>
[[nodiscard]] inline To narrow_cast(From&& value)
{
	return static_cast<To>(std::forward<From>(value));
}

template<typename To, typename From>
[[nodiscard]] inline std::enable_if_t<std::is_integral_v<To> && std::is_integral_v<From>, To>
  numeric_cast(From value)
{
	if (static_cast<From>(static_cast<To>(value)) != value) {
		throw std::system_error{ Error::numeric_narrowing };
	}
	return static_cast<To>(value);
}

} // namespace ysqlite3
