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

int main()
{
	try {
		ODF odf; odf = 5ui8;

		char data[100] = {};
		memset(data, 1, 100);
		MemoryDataStream write(data, 100);
		MemoryDataStream read((const char*)data, 100);
		odf.saveToMemory(write);

		ODF o2;
		o2.loadFromMemory(data, 100);

		cout << "\n\nresult: " << odf << " // " << o2 << "\n\n";

		return 0;
	}
	catch (std::runtime_error& err)
	{
		std::cout << "runtime error: " << err.what() << "\n";
		return 0;
	}
	catch (...)
	{
		std::cout << "unknown error";
		rethrow;
		return 0;
	}
}