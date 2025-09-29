#include "OOD.h"

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

using std::cout;

int main()
{
	cout.setf(std::ios::boolalpha);	

	try {
		OOD root;
		root.makeObjectList();
		OOD& o = root.insert();
		o.makeObject();
		o["fisch"] = 54Ui8;
		o["ff"] = 32Ui8;
		OOD& oo = root.insert();
		oo.makeObject();
		oo["fisch"] = 67ui8;
		oo["ff"] = 99ui8;

		cout.setf(std::ios::hex);
		cout << "root:\n" << root << "\n";

		root.saveToFile("root.ood");

		OOD ood;
		ood.loadFromFile("root.ood");
		cout << "ood: " << ood << "\n";
	}
	catch (std::exception& exp)
	{
		cout << "error: " << exp.what() << "\n";
		rethrow;
	}

	cout << "\n";
	system("pause");
	return 0;
}