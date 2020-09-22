// #include "base64_encoder.hpp"

// #include "base64.hpp"

// #include <gsl/gsl>

// using namespace ysqlite3::function;

// base64_encode::base64_encode() noexcept : function(1, true, false, text_encoding::utf8)
// {}

// void base64_encode::run(sqlite3_context* context, int argc, sqlite3_value** argv)
// {
// 	Expects(argc == 1);

// 	auto data = static_cast<const std::uint8_t*>(sqlite3_value_blob(argv[0]));
// 	auto size = sqlite3_value_bytes(argv[0]);
// 	auto out  = static_cast<char*>(sqlite3_malloc64(base64::required_encode_size(size)));
// 	auto end  = base64::encode(data, data + size, out);

// 	sqlite3_result_text(context, out, end - out, sqlite3_free);
// }

// base64_decode::base64_decode() noexcept : function(1, true, false, text_encoding::utf8)
// {}

// void base64_decode::run(sqlite3_context* context, int argc, sqlite3_value** argv)
// {
// 	Expects(argc == 1);

// 	auto data = sqlite3_value_text(argv[0]);
// 	auto size = sqlite3_value_bytes(argv[0]);
// 	auto out  = static_cast<std::uint8_t*>(sqlite3_malloc64(base64::max_decode_size(size)));
// 	auto end  = base64::decode(data, data + size, out);

// 	sqlite3_result_blob64(context, out, end - out, sqlite3_free);
// }
