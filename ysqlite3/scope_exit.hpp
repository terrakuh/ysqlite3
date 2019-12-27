#pragma once

#include <functional>
#include <memory>
#include <utility>

namespace ysqlite3 {

template<typename Lambda>
std::unique_ptr<void, std::function<void(void*)>> at_scope_exit(Lambda&& lambda)
{
    return { nullptr, [lambda](void*) { lambda(); } };
}

} // namespace ysqlite3