#include <random>
#include <functional>
#include "ODF.h"
using std::cout;

/*
* TODO:
* Fix this project.
* Learn Qt
* Become a sigma
* 
* Other stuff:
* make sure the list type inside a OBJLIST
* is equal, if some members are lists.
*/

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

int main()
{
	// goals:
	// odf = {
	//   { "key", value },
	//   { "key2", value2 }
	// } in fixed and mixed (detected at consutruction)
	// odf = map / unordered_map
	
	try {
		ODF odf;
		odf.makeList();

		odf.makeList(ODF::Type(ODF::TypeSpecifier::UINT));

		odf.push_back(5ui32);

		cout << "start:\n" << odf << "\n";

		char data[10000] = {};
		memset(data, 1, 100);
		MemoryDataStream write(data, 10000);
		if (odf.saveToMemory(write) != ODF::Status::Ok)
			cout << "penis\n";

		ODF o2;
		o2.loadFromMemory(data, 10000);

		cout << "\n\nresult:\n" << odf << "---\n" << o2 << "\n\n";

		if (odf.size() != o2.size())
			cout << "size mismatch\n";

		cout << "equal: " << std::boolalpha << (odf == o2);

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