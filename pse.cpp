#include "ODF.h"
#include "OdfInline.hpp"
#include "OdfUtil.hpp"

using namespace oi;

/*

json elemet example
{
	"appereance" : "string",
	"atomic_mass" : {
		"value" : double,
		"unit" : "string"
	},
	"cid" : "string",
	"cas" : "string",
	"image" : {
		"url" : "string",
		"caption" : "string"
	}
}

{ "name" : "Hydrogen", "symbol" : "H", "appereance": "colorless gas", "atomic_mass" : { "val" : 1.008, "unit" : "u" }, "cid" : "CID783", "cas" : "12385-13-6", "image" : { "url" : "https://upload.wikimedia.org/wikipedia/commons/8/83/Hydrogen_discharge_tube.jpg", "caption" : "<a href=\"https://commons.wikimedia.org/wiki/File:Hydrogen_discharge_tube.jpg\">Alchemist-hp (talk) (www.pse-mendelejew.de)</a>, FAL, via Wikimedia Commons" } }

*/

//void pse()
//{
//	constexpr Byte AtomicMassID = BuiltInType<2>();
//	constexpr Byte ImageID = BuiltInType<1>();
//	constexpr Byte ElementType = BuiltInType<0>();
//
//	const RawODF rodf = {
//		// define the image subtype
//		DEFTYPE, ImageID,
//		// type info
//		FXOBJ,
//		CSTR, // type
//		// keys
//		"url"strODF,
//		"caption"strODF,
//		END,
//
//		// define the atomic_mass subtype
//		DEFTYPE, AtomicMassID,
//		// type info
//		MXOBJ,
//		"unit"strODF, CSTR,
//		"val"strODF, DOUBLE,
//		END,
//
//		// a custom type at in-built id 0 will be defined that will server as element type
//		DEFTYPE, ElementType,
//
//		// define the element type
//		MXOBJ, // actual type
//		"name"strODF, CSTR,
//		"symbol"strODF, CSTR,
//		"appereance"strODF, CSTR, // "appereance": string
//		"cid"strODF, CSTR, // "cid": string
//		"cas"strODF, CSTR, // "cas": string
//		"atomic_mass"strODF, AtomicMassID, // "atomic_mass": custom type: atomic_mass
//		"image"strODF, ImageID, // "image": custom type: image
//		END,
//
//		// define the actual objects
//		ElementType,
//		"Hydrogen"strODF, // name
//		"H"strODF, // symbol,
//		"colorless gas"strODF, // appereance,
//		"CID783"strODF, // cid
//		"12385-13-6"strODF, // cas
//		"u"strODF, value<double>(1.008), // mass { "unit", value }
//		"https://upload.wikimedia.org/wikipedia/commons/8/83/Hydrogen_discharge_tube.jpg"strODF,
//		"<a href=\"https://commons.wikimedia.org/wiki/File:Hydrogen_discharge_tube.jpg\">Alchemist-hp (talk) (www.pse-mendelejew.de)</a>, FAL, via Wikimedia Commons"strODF,
//	};
//
//	auto [data, size] = CreateRawData(rodf);
//
//	std::ofstream out("old.odf", std::ios::binary);
//	out.write(data.get(), size);
//	out.close();
//
//	ODF element;
//	const char* dbg = data.get();
//	MemoryDataStream mds(data.get(), size);
//	element.loadFromMemory(mds);
//
//	std::cout << element << "\n";
//
//	element.saveToFile("element.odf");
//
//	size_t originalSize = std::filesystem::file_size("old.odf");
//	size_t newSize = std::filesystem::file_size("element.odf");
//	long long dif = newSize - originalSize;
//	if (!dif)
//		std::cout << "files are same size :)";
//	else
//		std::cout << "size mismatch: " << originalSize << " // " << newSize << " (" << (dif < 0 ? '-' : '+') << abs(dif) << ")\n";
//}

void pse()
{
	using std::cout;
	ODF number = 5;
	cout << "calculation: " << number * 5 << "\n";

	int i = number;

	ODF odf = {
		{"penis", 5},
		{"schwanz", 69},
		ODF::ODF_FOBJ
	};

	cout << odf;
}