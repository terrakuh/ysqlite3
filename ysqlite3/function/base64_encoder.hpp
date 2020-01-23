#ifndef YSQLITE3_FUNCTION_BASE64_ENCODE_HPP_
#define YSQLITE3_FUNCTION_BASE64_ENCODE_HPP_

#include "../base64.hpp"
#include "function.hpp"

#include <gsl/gsl>

namespace ysqlite3 {
namespace function {

class base64_encode : public function
{
public:
	base64_encode() noexcept : function(1, true, false, text_enconding::utf8)
	{}

protected:
	virtual void run(sqlite3_context* context, int argc, sqlite3_value** argv) override
	{
		Expects(argc == 1);

		auto data = static_cast<const std::uint8_t*>(sqlite3_value_blob(argv[0]));
		auto size = sqlite3_value_bytes(argv[0]);
		auto out  = static_cast<char*>(sqlite3_malloc64(base64::required_encode_size(size)));
		auto end  = base64::encode(data, data + size, out);

		sqlite3_result_text(context, out, end - out, sqlite3_free);
	}
};

class base64_decode : public function
{
public:
	base64_decode() noexcept : function(1, true, false, text_enconding::utf8)
	{}

protected:
	virtual void run(sqlite3_context* context, int argc, sqlite3_value** argv) override
	{
		Expects(argc == 1);

		auto data = sqlite3_value_text(argv[0]);
		auto size = sqlite3_value_bytes(argv[0]);
		auto out  = static_cast<std::uint8_t*>(sqlite3_malloc64(base64::max_decode_size(size)));
		auto end  = base64::decode(data, data + size, out);

		sqlite3_result_blob64(context, out, end - out, sqlite3_free);
	}
};

} // namespace function
} // namespace ysqlite3

#endif // !YSQLITE3_FUNCTION_BASE64_ENCODE_HPP_
