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
// implement in place construction of fixed objects

int main()
{
	// goal:
	// odf = {
	//   { "key", value },
	//   { "key2", value2 }
	// } in fixed and mixed (detected at consutruction)
	// odf = map / unordered_map
	
	try {
		/*ODF odf = {
			ODF::__OBJ,
			ODF::Pair{"peins", 19},
			ODF::Pair{"penis", 20}
		};*/

		ODF odf = { 5, 6, 7 };


		odf.push_back(5ui32); // Exception
		odf.push_back(6ui32);
		odf.push_back(7);

		odf[1] = "fisch"; // illegal ?

		cout << "start:\n" << odf << "\n";

		char data[10000] = {};
		memset(data, 1, 100);
		MemoryDataStream write(data, 10000);
		if (odf.saveToMemory(write) != ODF::Status::Ok)
			cout << "penis\n";

		ODF o2;
		o2.loadFromMemory(data, 10000); // BUG: THe load from memory function produces an invalid object. That means that the ODF::Type and the std::variat::index
		// doesn't actually match, which is why operator== throws a std::bad_variant_access

		cout << "\n\nresult:\n" << odf << "---\n" << o2 << "\n\n";

		if (odf.size() != o2.size())
			cout << "size mismatch\n";

		cout << "equal: " << std::boolalpha << (odf == o2) << "\n\n\n";

		return 0;
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