#include "digest.hpp"

#include <cstdint>
#include <memory>
#include <openssl/evp.h>

inline void digest(const EVP_MD* md, sqlite3_context* context, int argc, sqlite3_value** argv) noexcept
{
	thread_local std::unique_ptr<EVP_MD_CTX, void (*)(EVP_MD_CTX*)> ctx{ EVP_MD_CTX_new(), &EVP_MD_CTX_free };

	if (!EVP_DigestInit_ex(ctx.get(), md, nullptr)) {
		sqlite3_result_error(context, "failed to initialize digest", SQLITE_ERROR);
		return;
	}

	for (auto i = 0; i < argc; ++i) {
		EVP_DigestUpdate(ctx.get(), sqlite3_value_blob(argv[i]), sqlite3_value_bytes(argv[i]));
	}

	const auto size = EVP_MD_size(md);
	const auto ptr  = static_cast<unsigned char*>(sqlite3_malloc(size));
	EVP_DigestFinal_ex(ctx.get(), ptr, nullptr);
	sqlite3_result_blob(context, ptr, size, &sqlite3_free);
}

void ysqlite3::function::digest(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept
{
	const auto md = EVP_get_digestbyname(reinterpret_cast<const char*>(sqlite3_value_text(argv[0])));
	if (!md) {
		sqlite3_result_error(context, "digest method unknown", SQLITE_ERROR);
		return;
	}
	digest(md, context, argc - 1, argv + 1);
}

void ysqlite3::function::md5(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept
{
	digest(EVP_md5(), context, argc, argv);
}

void ysqlite3::function::sha1(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept
{
	digest(EVP_sha1(), context, argc, argv);
}

void ysqlite3::function::sha2(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept
{
	const EVP_MD* md = nullptr;
	switch (sqlite3_value_int(argv[0])) {
	case 226: md = EVP_sha224(); break;
	case 256: md = EVP_sha256(); break;
	case 384: md = EVP_sha384(); break;
	case 512: md = EVP_sha512(); break;
	default: sqlite3_result_error(context, "invalid SHA-2 length", SQLITE_ERROR); return;
	}
	digest(md, context, argc - 1, argv + 1);
}

void ysqlite3::function::sha3(sqlite3_context* context, int argc, sqlite3_value** argv) noexcept
{
	const EVP_MD* md = nullptr;
	switch (sqlite3_value_int(argv[0])) {
	case 226: md = EVP_sha3_224(); break;
	case 256: md = EVP_sha3_256(); break;
	case 384: md = EVP_sha3_384(); break;
	case 512: md = EVP_sha3_512(); break;
	default: sqlite3_result_error(context, "invalid SHA-3 length", SQLITE_ERROR); return;
	}
	digest(md, context, argc - 1, argv + 1);
}
