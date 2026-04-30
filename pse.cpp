#include "ODF.h"
#include "OdfInline.hpp"
#include "OdfUtil.hpp"
#include "json.hpp"

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

/* Epic test:
*  pse
*/

using Element = std::unordered_map<std::string, std::optional<std::shared_ptr<void>>>; // the void is an element
using DOM = std::shared_ptr<Element>;

void insert(Element& el, const std::string& key)
{
	size_t slash = key.find('/');
	if (slash == std::string::npos)
	{
		if (el.contains(key))
			throw 0;
		el[key] = std::nullopt;
	}
	else
	{
		// extract the actual (until the '/')
		std::string actualKey = key.substr(0, slash);
		// extract the leftover key, without the '/'
		std::string leftover = key.substr(slash + 1, key.size() - 1 - slash);
		// create and element if it doesn't exists
		if (!el.contains(actualKey))
			el[actualKey] = std::make_shared<Element>();

		insert(*std::static_pointer_cast<Element>(el[actualKey].value()), leftover);
	}
}

DOM constructDOM(const std::vector<std::string>& keys)
{
	DOM dom = std::make_shared<Element>();

	for (const auto& it : keys)
		insert(*dom, it);

	return dom;
}

std::ostream& operator<<(std::ostream& out, const std::vector<std::string>& vec)
{
	for (auto it : vec)
		out << "\r" << it << "              ";
	return out;
}

std::string readfile(std::string filename)
{
	std::ifstream in(filename, std::ios::binary | std::ios::ate);
	if (!in)
		return "";

	std::streamsize size = in.tellg();
	in.seekg(0, std::ios::beg);

	std::string result(size, '\0');
	in.read(result.data(), size);

	return result;
}

void cat(std::vector<std::string>& orig, const std::vector<std::string> add)
{
	orig.reserve(orig.size() + add.size());
	for (const auto& it : add)
		orig.push_back(it);
}

using json = nlohmann::json;
static std::vector<std::string> getSubKeys(json& j, std::string prefix = "")
{
	if (!j.is_object())
	{
		std::cout << "error: not an object.\n";
		return {};
	}
	if (prefix.size())
		if (prefix[prefix.size() - 1] != '/')
			prefix += '/';
	std::vector<std::string> keys;
	for (auto it = j.begin(); it != j.end(); it++)
	{
		if (it.value().is_object())
			cat(keys, getSubKeys(it.value(), prefix + it.key()));
		else
			keys.push_back(prefix + it.key());
	}
	std::cout << keys;
	return keys;
}

bool operator==(const std::vector<std::string>& left, const std::vector<std::string>& right)
{
	if (left.size() != right.size())
		return false;
	for (size_t i = 0; i < left.size(); i++)
		if (left[i] != right[i])
			return false;
	return true;
}

std::vector<std::string> flat(const std::vector<std::pair<std::string, std::vector<std::string>>>& vec)
{
	size_t size = 0;
	for (const auto& it : vec)
		size += it.second.size();

	std::vector<std::string> result;
	result.reserve(size);

	for (const auto& it : vec)
		for (const auto& it2 : it.second)
			result.push_back(it2);

	std::cout << "flat size: " << result.size() << "\n";
	return result;
}

std::vector<std::string> removeDuplicates(const std::vector<std::string>& vec)
{
	std::vector<std::string> result;
	for (const auto& it : vec)
		if (std::ranges::find(vec, it) != vec.end())
			result.push_back(it);
	return result;
}

static json get(const json& js, const std::string& key)
{
	size_t slash = key.find('/');
	if (slash == std::string::npos)
		return js.at(key);
	
	std::string first = key.substr(0, slash);
	std::string rest = key.substr(slash + 1, key.size() - 1 - slash);
	return get(js.at(first), rest);
}

static void set(ODF& odf, const std::string& key, const json& val)
{
	size_t slash = key.find('/');
	if (slash == std::string::npos)
	{
		odf[key] = val;
		return;
	}

	std::string first = key.substr(0, slash);
	std::string last = key.substr(slash + 1, key.size() - 1 - slash);
	set(odf[first], last, val);
}

static void set(json& js, const std::string& key, const json& val)
{
	size_t slash = key.find('/');
	if (slash == std::string::npos)
	{
		js[key] = val;
		return;
	}

	std::string first = key.substr(0, slash);
	std::string last = key.substr(slash + 1, key.size() - 1 - slash);
	set(js[first], last, val);
}

static void copyKey(json& dest, const std::string& key, const json& source)
{
	set(dest, key, get(source, key));
}

static void copyKey(ODF& dest, const std::string& key, const json& source)
{
	set(dest, key, get(source, key));
}

void print(const std::vector<std::string>& vec)
{
	for (const auto& it : vec)
		std::cout << it << "\n";
}

void pse()
{
	using std::cout;

	// check wether all keys exist consistently over all elements.

	using List = std::vector<std::string>;
	std::vector<std::string> keys;
	
	using json = nlohmann::json;
	json jpse = json::parse(readfile("test/pse.json"));
	
	std::vector<std::pair<std::string, List>> keyLists;
	keyLists.reserve(jpse.size());
	for (auto it = jpse.begin(); it != jpse.end(); it++)
	{
		cout << "\rcollecting keys of \"" << it.key() << "\"";
		keyLists.push_back(std::make_pair(it.key(), getSubKeys(it.value())));
		if (keyLists[keyLists.size() - 1].second.empty())
			cout << "warning: keyList of \"" << it.key() << "\" is empty\n";
	}
	cout << "\rall keys collected                \n";

	List allKeys = removeDuplicates(flat(keyLists));

	List genericKeys; // keys which exist in every element

	for (const auto& it : allKeys)
	{
		bool existsInAll = true;
		for (const auto& keyList : keyLists)
		{
			if (std::ranges::find(keyList.second, it) == keyList.second.end())
			{
				existsInAll = false;
				break;
			}
		}
		if (existsInAll)
		{
			// add key if it wasn't added already
			if (std::ranges::find(genericKeys, it) == genericKeys.end())
				genericKeys.push_back(it);
		}
	}

	cout << "Number of generic keys: " << genericKeys.size() << "\n";
	cout << "===== Result =====\n";
	print(genericKeys);

	DOM dom = constructDOM(genericKeys);

	// copy only the needed keys
	json processedPSE;
	ODF odfPSE;
	for (auto it = jpse.begin(); it != jpse.end(); it++)
	{
		json& element = it.value();
		json& processedElement = processedPSE[it.key()];
		for (const auto& key : genericKeys)
		{
			copyKey(processedElement, key, element);
			copyKey(odfPSE, key, element);
		}
	}

	std::ofstream out("test/ppse.json");
	std::string dump = processedPSE.dump();
	out.write(dump.c_str(), dump.size());
	out.close();

	cout << "cosntructed processed source json\n";
	cout << "===== ODF ======\n" << odfPSE << "\n";


	// transfer structure into a ODF type.

	// slice the selected keys into a seperate json object

	// save json object without formatting

	// save odf

	// compare size
}