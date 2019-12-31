#define GSL_THROW_ON_CONTRACT_VIOLATION

#include <backward.hpp>
#include <iostream>
#include <typeinfo>
#include <ysqlite3/database.hpp>
#include <ysqlite3/exception/base_exception.hpp>
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
	summi() : function(2, true, true, text_enconding::utf8)
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

int main(int args, char** argv)
{
	using namespace backward;
	StackTrace st;
	st.load_here(32);
	backward::SignalHandling sh;

	/*Printer p;
	p.print(st);*/
	try {
		auto v = std::make_shared<vfs::layer::layered_vfs<>>(
		    std::make_shared<vfs::sqlite3_vfs_wrapper<>>(vfs::find_vfs(nullptr), "myvfs"), "myvfs");

		v->add_layer<rot13_layer>();

		vfs::register_vfs(v, true);
		// auto _ = gsl::finally([&v] { vfs::vfs::unregister_vfs(&v); });

		database db;

		db.open("D:/test.db");
		// db.register_function<summi>("summi");
		db.execute(R"(CREATE TABLE IF NOT EXISTS tast(noim text not null); INSERT INTO tast(noim) VALUES('heyho'),
		 ('Musik');)");

		//		db.prepare_statement(R"(INSERT INTO tast(noim) VALUES(?))").bind(1, 65).finish();

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