#ifndef YSQLITE3_SPAN_HPP_
#define YSQLITE3_SPAN_HPP_

namespace ysqlite3 {

template<typename Iterator>
class span
{
public:
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
