#ifndef YSQLITE3_EXCEPTION_BASE_EXCEPTION_HPP_
#define YSQLITE3_EXCEPTION_BASE_EXCEPTION_HPP_

#include <exception>
#include <string>

#ifndef __FUNCTION_NAME__
#    ifdef WIN32
#        define __FUNCTION_NAME__ __FUNCTION__
#    else
#        define __FUNCTION_NAME__ __func__
#    endif
#endif

#define YSQLITE_THROW(e, ...) throw e(__VA_ARGS__, __FILE__, __FUNCTION_NAME__, __LINE__);

namespace ysqlite3 {
namespace exception {

class base_exception : public std::exception
{
public:
    base_exception(const std::string& msg, const char* file, const char* function, int line)
        : _msg(msg), _file(file), _function(function ? function : "*no func*"), _line(line)
    {}
    virtual const char* what() const noexcept override
    {
        return _msg.c_str();
    }
    const std::string& file() const noexcept
    {
        return _file;
    }
    const std::string& function() const noexcept
    {
        return _function;
    }
    int line() const noexcept
    {
        return _line;
    }

private:
    std::string _msg;
    std::string _file;
    std::string _function;
    int _line;
};

} // namespace exception
} // namespace ysqlite3

#endif