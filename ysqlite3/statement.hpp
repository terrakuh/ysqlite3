#pragma once

namespace ysqlite3 {

class statement
{
public:
    void bind();
    template<typename T>
    void bind(T&& value)
    {}
};

} // namespace ysqlite3