#include <iostream>
#include <typeinfo>
#include <ysqlite3/database.hpp>
#include <ysqlite3/exception/base_exception.hpp>
#include <ysqlite3/function/base64_encoder.hpp>
#include <ysqlite3/function/regexp.hpp>
#include <ysqlite3/vfs/layer/layered_vfs.hpp>
#include <ysqlite3/vfs/sqlite3_vfs_wrapper.hpp>
#include <ysqlite3/vfs/vfs.hpp>

using namespace ysqlite3;

inline int f(void*, int columns, char** values, char** names)
{
	std::cout << "got " << columns << " columns\n";

	for (auto i = 0; i < columns; ++i) {
		std::cout << names[i] << "=" << values[0] << std::endl;
	}

	return SQLITE_OK;
}

class summi : public function::function
{
public:
	summi() : function(2, true, true, text_encoding::utf8)
	{}

protected:
	virtual void run(sqlite3_context* context, int argc, sqlite3_value** argv) override
	{
		sqlite3_int64 s = 0;

		for (auto i = 0; i < argc; ++i) {
			s += sqlite3_value_int64(argv[i]);
		}

		sqlite3_result_int64(context, s);
	}
};

class rot13_layer : public vfs::layer::layer
{
public:
	virtual void encode(gsl::span<gsl::byte> buffer, sqlite3_int64) override
	{
		for (auto& i : buffer) {
			i = static_cast<gsl::byte>(static_cast<unsigned char>(i) + 13);
		}
	}
	virtual void decode(gsl::span<gsl::byte> buffer, sqlite3_int64) override
	{
		for (auto& i : buffer) {
			i = static_cast<gsl::byte>(static_cast<unsigned char>(i) - 13);
		}
	}
};

class super : public vfs::layer::layered_file
{
public:
	using layered_file::layered_file;

	virtual void file_control(file_cntl operation, void* arg) override
	{
		if (operation == file_cntl_pragma) {
			auto name  = static_cast<char**>(arg)[1];
			auto value = static_cast<char**>(arg)[2];

			if (!std::strcmp("print", name)) {
				printf("#: %s\n", value ? value : "null");

				return;
			}
		}

		layered_file::file_control(operation, arg);
	}
};

int main(int args, char** argv)
{
	using namespace backward;
	StackTrace st;
	st.load_here(32);
	backward::SignalHandling sh;

	/*Printer p;
	p.print(st);*/
	try {
		auto v = std::make_shared<vfs::layer::layered_vfs<vfs::sqlite3_vfs_wrapper<>, super>>(
		    vfs::find_vfs(nullptr), "myvfs");

		v->add_layer<rot13_layer>();

		vfs::register_vfs(v, true);
		// auto _ = gsl::finally([&v] { vfs::vfs::unregister_vfs(&v); });

		database db;

		db.open("test.db");

		db.register_function<function::base64_encode>("base64_encode");
		db.register_function<function::base64_decode>("base64_decode");
		db.register_function<ysqlite3::function::regexp>("regexp");

		// db.register_function<summi>("summi");
		// db.execute(R"(pragma print("hello, world");)");

		db.execute(R"(
	CREATE TABLE IF NOT EXISTS tast(noim text);
	INSERT INTO tast(noim) VALUES('heyho'), ('Musik'), (NULL);
)");

		auto stmt = db.prepare_statement("select * from tast");
		results r;

		while ((r = stmt.step())) {
			for (auto i : stmt.columns()) {
				auto text = r.text(i.c_str());

				std::cout << i << ": " << (!text ? "<null>" : text) << std::endl;
			}
		}

	} catch (const std::exception& e) {
		std::cerr << "exception caught (" << typeid(e).name() << ")";

		if (dynamic_cast<const exception::base_exception*>(&e)) {
			auto& ex = static_cast<const exception::base_exception&>(e);

			std::cerr << " at " << ex.file() << ":" << ex.line() << " from " << ex.function();
		}

		std::cerr << "\n\t" << e.what() << std::endl;
	}

	return 0;
}
