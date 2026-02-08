#include <random>
#include <functional>
#include "ODF.h"
using std::cout;

double random()
{
	static std::mt19937 gen(std::random_device{}());
	std::uniform_real_distribution<double> dist(0.0, 1.0);
	return dist(gen);
}

double random(double from, double to)
{
	double range = to - from;
	return random() * range + from;
}

template<typename T>
void fill(std::vector<T>& vec, size_t size, std::function<T(void)> functor)
{
	vec.resize(size);
	for (T& t : vec)
		t = functor();
}

// TODO

// planned features
// typedef support
// using types from exteral files

// current debug TODO 

int main()
{
	try {

		// Data type test code
		//ODF odf = 5;

		//cout << "start:\n" << odf << "\n";

		//odf["penis"s] = 6;

		//char data[10000] = {};
		//memset(data, 1, 100);
		//MemoryDataStream write(data, 10000);
		//if (odf.saveToMemory(write) != ODF::Status::Ok)
		//	cout << "penis\n";

		//ODF o2;
		//o2.loadFromMemory(data, 10000); // BUG: THe load from memory function produces an invalid object. That means that the ODF::Type and the std::variat::index
		//// doesn't actually match, which is why operator== throws a std::bad_variant_access

		//cout << "\n\nresult:\n" << odf << "---\n" << o2 << "\n\n";

		//if (odf.size() != o2.size())
		//	cout << "size mismatch\n";

		//cout << "equal: " << std::boolalpha << (odf == o2) << "\n\n\n";

		//return 0;

		// VTYPE test code
		ODF odf;


		// example: define the type 0x69 as unsigned int and use it. works
		//const unsigned char data[] = {
		//	0x49ui8, // DEFTYPE, SSS1
		//	0x69ui8, // SSS1ID0x69
		//	0x23ui8, // Type: unsigned int

		//	0x69ui8, // USETYPE
		//	0x69ui8, // ID = 0x69
		//	0x5, 0x5, 0x5, 0x5 // data
		//};

		// example: define the type 0x69 as extension to the built in types. as unsigned int and use it.
		const unsigned char data[] = {
			0x9ui8, // DEFTYPE, SSS1
			0x6Eui8, // SSS0ID0x6E
			0x23ui8, // Type: unsigned int

			0x29ui8, // USETYPE
			0x6Eui8, // ID = 0x6E
			0x5, 0x5, 0x5, 0x5 // data
		};


		odf.loadFromMemory((const char*)data, sizeof(data));

		cout << "result:\n" << odf << "\n";
	}
	catch (std::runtime_error& err)
	{
		std::cout << "runtime error: " << err.what() << "\n";
		rethrow;
		return 0;
	}
	/*catch (...)
	{
		std::cout << "unknown error";
		rethrow;
		return 0;
	}*/
}