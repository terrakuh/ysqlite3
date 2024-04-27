#pragma once

#include <type_traits>

namespace ysqlite3::util {

template<typename Type, template<typename...> typename Template>
struct TemplatedOf : std::false_type
{};

template<template<typename...> typename Template, typename... Types>
struct TemplatedOf<Template<Types...>, Template> : std::true_type
{};

} // namespace ysqlite3::util
