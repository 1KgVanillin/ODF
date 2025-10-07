#include "ODF.h"
#include "OOD.h"

bool OOD::enable_string_obfuscation_static = enable_string_obfuscation_default;

bool OOD::isObjectArray(std::unordered_map<std::string, Type>* fields) const
{
	//this functions needs to create a list of keys, and then store them into list. then check if those keys exist in every child object
	const std::vector<OOD>& list = std::get<std::vector<OOD>>(data);
	if (!list.size())
		return true;

	std::vector<std::string> keys;
	for (const OOD& ood : list)
	{
		const std::unordered_map<std::string, OOD>& map = ood.get<std::unordered_map<std::string, OOD>>();
		for (auto it = map.begin(); it != map.end(); it++)
			if (std::find(keys.begin(), keys.end(), it->first) == keys.end())
				keys.push_back(it->first); // add the key if it doesnt exist in the list of keys
	}
	
	// now check every object if it contains all keys.
	for (const OOD& ood : list)
	{
		const std::unordered_map<std::string, OOD>& map = ood.get<std::unordered_map<std::string, OOD>>();
		for (const std::string& key : keys)
			if (map.find(key) == map.end()) // key not found, no objectarray
				return false;
	}

	//get the types
	if (fields)
	{
		const std::unordered_map<std::string, OOD>& map = list.begin()->get<std::unordered_map<std::string, OOD>>();
		for (auto it = map.begin(); it != map.end(); it++)
			(*fields)[it->first] = it->second.getSaveType();
	}

	//every object cpontains the needed keys. Now check all the types
	for (const OOD& ood : list)
	{
		const std::unordered_map<std::string, OOD>& map = ood.get<std::unordered_map<std::string, OOD>>();
		for (const std::string& key : keys) // check if this key in this object is the needed type
			if (map.at(key).getSaveType() != (*fields)[key])
				return false;
	}

	return true;
}

void OOD::makeUnreadable(std::string& str)
{
	//invert bits
	for (char& c : str)
		c = ~c;
	//swap some stuff
	for (size_t i = 0; i < str.size() - 1; i += 2)
		std::swap(str[i], str[i + 1]);
}

void OOD::makeReadable(std::string& str)
{
	//swap some stuff
	for (size_t i = 0; i < str.size() - 1; i += 2)
		std::swap(str[i], str[i + 1]);
	//invert bits
	for (char& c : str)
		c = ~c;
}

std::string OOD::readFile(std::string file, bool silent)
{
	std::ifstream in;
	in.open(file, std::ios::ate);
	if (in.fail())
	{
		if (!silent)
			std::cout << "failed to open file \"" << file << "\" for reading.\n";
		return "";
	}

	//get size
	size_t size = in.tellg();

	//set cursor to start
	in.seekg(std::ios::beg);

	//allocate memory
	std::string content;
	content.resize(size);

	//read file
	in.read(content.data(), size);
	if (in.fail())
	{
		if (!silent)
			std::cout << "failed to read file \"" << file << "\": reading error\n";
		return "";
	}
	
	in.close();
	return content;
}

bool OOD::writeFile(std::string file, const char* data, size_t size, bool silent)
{
	std::ofstream out;
	out.open(file);
	if (out.fail())
	{
		if (!silent)
			std::cout << "failed to open file \"" << file << "\" for writing.\n";
		return false;
	}

	out.write(data, size);
	if (out.fail())
	{
		if (!silent)
			std::cout << "failed to write file \"" << file << "\".\n";
		return false;
	}

	out.close();
	return true;
}

void OOD::saveAsObject(MemoryDataStream& mem)
{
	// mixed objects and non mixed objects differ only in their header.
	// they have their own code though because iteration functions differently (vector vs map)
	
	// in both cases, all elements are iterated and saved. the keys are iterated from the map in the header object and then used in the actual unordered_map to ensure that the order is correct.

	// TODO:
	// Decide if the ComplexType component of the MixedObjectSpecifier should be an optional. It is technically only needed for loading.
	// After that, the ComplexType header of the actual object can be used. This would ensure, that no mismatches happen between them.

	if (header.type == Type::MXOBJ)
	{ // mixed

	}
}

std::string OOD::getTypeStr(const size_t& data)
{
	switch (data)
	{
	case 0: return "NULL";
	case 1: return "BYTE";
	case 2: return "UBYTE";
	case 3: return "SHORT";
	case 4: return "USHORT";
	case 5: return "INT";
	case 6: return "UINT";
	case 7: return "LONG";
	case 8: return "ULONG";
	case 9: return "FLOAT";
	case 10: return "DOUBLE";
	case 11: return "CSTR";
	case 12: return "WSTR";

	case 13: return "MLIST";

	case 14: return "MOBJ";
	default:
		return "[unknown wtf? -> typeindex=" + std::to_string(data) + "]";
	}
}

void OOD::enableStringObfuscation(bool justForThis)
{
	if (justForThis)
		enable_string_obfuscation = true;
	else
		enable_string_obfuscation_static = true;
}

void MemoryDataStream::enableStringObfuscation(bool obfuscation)
{
	obfuscated = obfuscation;
}

void MemoryDataStream::disableStringObfuscation()
{
	enableStringObfuscation(false);
}

void OOD::disableStringObfuscation(bool justForThis)
{
	if (justForThis)
		enable_string_obfuscation = false;
	else
		enable_string_obfuscation_static = false;
}

bool OOD::isList() const
{
	return data.index() == 13;
}

bool OOD::isObject() const
{
	return data.index() == 14;
}

void OOD::unforceCompression()
{
	forcecompression = false;
}

bool OOD::loadFromMemory(const char* mem, size_t size)
{
	// reset object
	forcecompression = false;
	expectedType = Type::NULL;
	forcetype = false;
	forcedType = Type::NULL;

	MemoryDataStream memstream(mem, size);
	
	try {
		TypeSpecifier type = memstream.read();
		loadWithOutType(type, memstream);
		return true;
	}
	catch (std::out_of_range& excep)
	{
		std::cout << "loadFromMemory failed (out of range):" << excep.what() << "\n";
		return false;
	}
	catch (MemoryDataStream::ReadOnlyViolation& excep)
	{
		std::cout << "loadFromMemory failed (RdOnlyViolation): " << excep.what() << "\n";
		return false;
	}
}

void OOD::loadFromMemory(MemoryDataStream& mem)
{
	// reset object
	forcecompression = false;
	expectedType = Type::NULL;
	forcetype = false;
	forcedType = Type::NULL;


	TypeSpecifier type = mem.read();
	loadWithOutType(type, mem);
}

void OOD::loadWithOutType(TypeSpecifier t, MemoryDataStream& mem)
{
	
}

MemoryDataStream OOD::saveToStream()
{
	MemoryDataStream stream(getNeededSize());
	saveToMemory(stream);
	return stream.move(); // stream needs to be moved, otherwise it the destructor will delete the data when the function goes out of scop
}

char* OOD::saveToMemory(size_t& size)
{
	size = getNeededSize();
	char* memdata = new char[size];
	MemoryDataStream mem = memdata;
	mem.setMaxSize(size);
	saveToMemory(mem);
	return memdata;
}

void OOD::saveToMemory(MemoryDataStream& mem, bool withoutType)
{
	if (!withoutType)
		header.saveToMemory(mem);

	switch (header.type)
	{
		// string types
	case Type::CSTR:
		mem.writeStr(std::get<std::string>(data));
		return;
	case Type::WSTR:
		mem.writeWstr(std::get<std::wstring>(data));
		return;
		// complex types
	case Type::FXOBJ:
	case Type::MXOBJ:
		return saveAsObject(mem); // void lol

	case Type::FXLST:
	case Type::MXLST:
		return saveAsList(mem);

	default: // primitive type
		mem.write(data);
		return;
	}
}

OOD::Type OOD::getSaveType() const
{
	
}

size_t OOD::getNeededSize() const
{
	//To save a primitive type you just do
	//1byte specifier, data.
	
	if (!isList() && !isObject())
		return getPrimitiveDataSize(data.index()) + 1; // 1 byte for the type specifier
	// non primitive types:
	// compression means non-mixed

	size_t neededSize = 1; // 1 for the type specifier

	//array
	if (auto ptr = std::get_if<std::vector<OOD>>(&data))
	{
		if (!ptr->size())
			return 0;
		if (forcecompression)
		{ // non mixed
			neededSize += 1; // 1 for the data type
			//select the right array type, LIST (1byte size), LLIST (2byte size), MXLIST (4byte size)
			if (ptr->size() < LIST_OVERFLOW_SIZE)
				neededSize += 1;
			else if (ptr->size() < LLIST_OVERFLOW_SIZE)
				neededSize += 2;
			else if (ptr->size() < MXLIST_OVERFLOW_SIZE)
				neededSize += 4;
			else // bro wtf as if the array would be bigger
			{
				THROW InvalidArraySize();
			}

			return neededSize + (getPrimitiveDataSize(expectedType) * ptr->size());
		}
		else
		{ // mixed
			// for a mixed array there is the Mixedarray type specifier, then just type and data until there is a terminator
			for (const OOD& ood : *ptr)
				neededSize += ood.getNeededSize();
			return neededSize + 1; // one additional byte for the array terminator
		}
	}

	if (auto ptr = std::get_if<std::unordered_map<std::string, OOD>>(&data))
	{
		if (!ptr->size())
			return 0;
		if (forcecompression)
		{ // non mixed
			// THe format is basically OBJ TYPE KEY NULL DATA
			for (auto it = ptr->begin(); it != ptr->end(); it++)
			{
				neededSize += it->first.size() + 1;
				neededSize += it->second.getNeededSize();
			}
			return neededSize + 1; // for the data type
		}
		else
		{ // mixed
			// basically the format is just key (cstr with null terminator) + type + value as often as needed
			// so basically every value has the size key.size() + 2 + value.size()
			for (auto it = ptr->begin(); it != ptr->end(); it++)
			{
				neededSize += it->first.size() + 2; // NULL and data type
				neededSize += it->second.getNeededSize();
			}

			return neededSize + 1; // 1 byte for the end terminator
		}
	}
}

size_t OOD::getPrimitiveDataSize(size_t type) const
{
	switch (type)
	{
	case Type::CHAR:
	case Type::UCHAR:
		return 1;
	case Type::SHORT:
	case Type::USHORT:
		return 2;
	case Type::INT:
	case Type::UINT:
	case Type::FLOAT:
		return 4;
	case Type::LONG:
	case Type::ULONG:
	case Type::DOUBLE:
		return 8;
	case Type::CSTR:
		return std::get<std::string>(data).size() + 1; // 1 byte for the null terminator
	case Type::WSTR:
		return std::get<std::wstring>(data).size() + 2; // 2 bytes null terminator
	}
}

size_t OOD::getSizeWithoutType() const
{
	return getNeededSize() - 1; // -1 byte for the type
}

size_t OOD::getValueSize() const
{
	if (!isObject())
		return 0;
	size_t neededSize = 0;
	for (const std::pair<std::string, OOD>& element : std::get<std::unordered_map<std::string, OOD>>(data))
	{
		if (element.second.isObject())
			neededSize += element.second.getValueSize();
		else if (element.second.isList())
			neededSize += element.second.getNeededSize() - 1; // minus one byte for the type. The type of the lists need to be equal
		else
			neededSize += getPrimitiveDataSize(element.second.data.index());
	}
	return neededSize;
}

bool OOD::loadFromFile(std::string filename)
{
	std::string file = readFile(filename);
	loadFromMemory(file.data(), file.size());
	return true;
}

bool OOD::saveToFile(std::string filename)
{
	MemoryDataStream mem(getNeededSize());
	saveToMemory(mem);
	return writeFile(filename, mem.data(), mem.size());
} // mem deallocates data when going out of scope

void OOD::forceCompression(TypeSpecifier t)
{
	size_t previousExpectedType = expectedType; // save for restoring in case of failure
	expectedType = t;
	if (!isCompressable())
	{
		expectedType = previousExpectedType; // restore
		THROW NotCompressable();
	}
	forcecompression = true;
}

bool OOD::isCompressable()
{
	if (auto ptr = std::get_if<std::vector<OOD>>(&data))
	{
		for (OOD& it : *ptr)
			if (it.data.index() != expectedType)
				return false;
		return true;
	}

	if (auto ptr = std::get_if<std::unordered_map<std::string, OOD>>(&data))
	{
		for (auto it = ptr->begin(); it != ptr->end(); it++)
			if (it->second.data.index() != expectedType)
				return false;
		return true;
	}
	return true; // primitive type. It will enable forcecompression and apply the compression when the object is turned into an object or array
}

bool OOD::isCompressed() const
{
	return forcecompression;
}

void OOD::makeType(TypeSpecifier t, TypeSpecifier subtype)
{
	switch (t)
	{
	case Type::CHAR:
		makeType<INT_8>();
		break;
	case Type::UCHAR:
		makeType<UINT_8>();
		break;
	case Type::SHORT:
		makeType<INT_16>();
		break;
	case Type::USHORT:
		makeType<UINT_16>();
		break;
	case Type::INT:
		makeType<INT_32>();
		break;
	case Type::UINT:
		makeType<UINT_32>();
		break;
	case Type::LONG:
		makeType<INT_64>();
		break;
	case Type::ULONG:
		makeType<UINT_64>();
		break;
	case Type::FLOAT:
		makeType<float>();
		break;
	case Type::DOUBLE:
		makeType<double>();
		break;
	case Type::CSTR:
		makeType<std::string>();
		break;
	case Type::WSTR:
		makeType<std::wstring>();
		break;
	case Type::MXLST:
		makeType<std::vector<OOD>>();
		unforceCompression();
		break;
	case Type::MXOBJ:
		makeType<std::unordered_map<std::string, OOD>>();
		unforceCompression();
		break;
	case Type::FXLST:
		makeType<std::vector<OOD>>();
		forceCompression(subtype);
		break;
	case Type::FXOBJ:
		makeType<std::unordered_map<std::string, OOD>>();
		forceCompression(subtype);
		break;
	default:
		THROW SwitchCaseDefaultReached("unhandled type in makeType(type, subtype), propably NULL type");
	}
}

void OOD::makeObject()
{
	if (std::holds_alternative<std::unordered_map<std::string, OOD>>(data)) return; // already right type
	data.emplace<std::unordered_map<std::string, OOD>>();
}

void OOD::makeList()
{
	if (std::holds_alternative<std::vector<OOD>>(data)) return; // already right type
	data.emplace<std::vector<OOD>>();
}

void OOD::resize(size_t newSize)
{
	std::get<std::vector<OOD>>(data).resize(newSize);
}

OOD& OOD::insert(std::string key)
{
	return this->operator[](key);
}

OOD& OOD::insert()
{
	std::vector<OOD>& list = std::get<std::vector<OOD>>(data);
	list.emplace_back();
	return list[list.size() - 1];
}

void OOD::erase(std::vector<size_t> indieces)
{
	for (size_t i : indieces)
		erase(i);
}

void OOD::erase(size_t index)
{
	std::vector<OOD>& list = std::get<std::vector<OOD>>(data);
	list.erase(list.begin() + index);
}

void OOD::erase(std::string key)
{
	std::unordered_map<std::string, OOD>& map = std::get<std::unordered_map<std::string, OOD>>(data);
	map.erase(key);
}

std::vector<OOD>* OOD::getListAccess()
{
	return &std::get<std::vector<OOD>>(data);
}

std::unordered_map<std::string, OOD>* OOD::getMapAccess()
{
	return &std::get<std::unordered_map<std::string, OOD>>(data);
}

OOD& OOD::operator[](size_t index)
{
	if (!std::holds_alternative<std::vector<OOD>>(data))
	{
		THROW BadAccessOperator("object is not a vector");
	}
	return std::get<std::vector<OOD>>(data)[index];
}

OOD& OOD::operator[](std::string key)
{
	return (*this)[key.c_str()];
}

OOD& OOD::operator[](const char* key)
{
	if (!std::holds_alternative<std::unordered_map<std::string, OOD>>(data))
	{
		THROW BadAccessOperator("object is not an object");
	}

	std::unordered_map<std::string, OOD>& map = std::get<std::unordered_map<std::string, OOD>>(data); // get map object

	//check if the key is a insertion
	if (!map.count(key)) // if key is new
	{
		if (map[key].forcetype = forcecompression) // and compression is forced
			map[key].forcedType = expectedType; // then forcedType will be expectedType
	}

	return map[key];
}

OOD::OOD()
{
	enable_string_obfuscation = enable_string_obfuscation_static;
	unforceCompression();
	data = nullptr;
	forcetype = false;
	forcedType = 0;
}

OOD::BadImplicitConversion::BadImplicitConversion(size_t sourceType, size_t destType, std::string msg)
{
	std::string types = "("; // will be like "(int -> std::string)"
	types += getTypeStr(sourceType);
	types += " -> ";
	types += getTypeStr(destType);
	types += ")";

	if (msg != "")
		this->msg = msg + " " + types;
	else
		this->msg = "BadImplicitConversion " + types;
}

const char* OOD::Exception::what() const noexcept
{
	return msg.c_str();
}

OOD::BadArgumentType::BadArgumentType(size_t wantedArgumentType, size_t realArgumentType, std::string message)
{
	std::string stderrormsg = "[BadArgumentType: Wanted " + getTypeStr(wantedArgumentType)
		+ ", Got " + getTypeStr(realArgumentType) + "]";
	msg = message + (message != "" ? " " : "") + stderrormsg;
}

OOD::BadArgumentType::BadArgumentType(std::vector<size_t> wantedArgumentTypes, size_t realArgumentType, std::string message)
{
	std::string stderrormsg = "[BadArgumentType: Wanted (";
	for (size_t i = 0; i < wantedArgumentTypes.size(); i++)
	{
		stderrormsg += getTypeStr(wantedArgumentTypes[i]);
		if (i != wantedArgumentTypes.size() - 1)
			stderrormsg += ", ";
	}
	stderrormsg += ") Got " + getTypeStr(realArgumentType) + "]";
	msg = message + (message != "" ? " " : "") + stderrormsg;
}

OOD::BadAccessOperator::BadAccessOperator(std::string message)
{
	msg = message;
}

std::ostream& operator<<(std::ostream& out, OOD& ood)
{
	out.setf(std::ios::boolalpha);
	if (ood.data.index() == 0)
	{
		out << "[nullvalue]";
		return out;
	}

	if (auto ptr = std::get_if<std::vector<OOD>>(&ood.data))
	{
		out << "LIST\t";
		out << "[mixed:" << !ood.isCompressed() << "]\n";
		for (size_t i = 0; i < ptr->size(); i++)
			out << "[" << i << "] " << (*ptr)[i] << "\n";
		return out;
	}

	if (auto ptr = std::get_if<std::unordered_map<std::string, OOD>>(&ood.data))
	{
		out << "OBJ\t";
		out << "[mixed:" << !ood.isCompressed() << "]\n";
		for (auto it = ptr->begin(); it != ptr->end(); it++)
			out << "[" << it->first << "] " << it->second << "\n";
		return out;
	}

	using Type = OOD::Type;
	switch (ood.data.index())
	{
	case Type::CHAR:
		out << ood.get<INT_8>();
		break;
	case Type::UCHAR:
		out << (unsigned int)ood.get<UINT_8>();
		break;
	case Type::SHORT:
		out << ood.get<INT_16>();
		break;
	case Type::USHORT:
		out << ood.get<UINT_16>();
		break;
	case Type::INT:
		out << ood.get<INT_32>();
		break;
	case Type::UINT:
		out << ood.get<UINT_32>();
		break;
	case Type::LONG:
		out << ood.get<INT_64>();
		break;
	case Type::ULONG:
		out << ood.get<UINT_64>();
		break;
	case Type::FLOAT:
		out << ood.get<float>();
		break;
	case Type::DOUBLE:
		out << ood.get<double>();
		break;
	case Type::CSTR:
		out << ood.get<std::string>();
		break;
	case Type::WSTR:
		out << ood.get<std::wstring>();
		break;
	default:
		out << "{?}";
	}

	out << " (" << ood.getTypeStr(ood.data.index()) << ")";
	return out;
}

bool operator==(const OOD& ood1, const OOD& ood2)
{
	// type must match
	if (ood1.getSaveType() != ood2.getSaveType())
		return false;

	// value must match
	if (ood1.data != ood2.data)
		return false;
	return true;
}

std::ostream& operator<<(std::ostream& out, std::wstring wstr)
{
	out << "L\"";
	for (wchar_t w : wstr)
		out << (char)w;
	out << "\"";
	return out;
}

OOD::BadConstructionType::BadConstructionType(size_t wantedArgumentType, size_t realArgumentType, std::string message)
{
	std::string stderrormsg = "[BadConstructionType: Wanted " + getTypeStr(wantedArgumentType)
		+ ", Got " + getTypeStr(realArgumentType) + "]";
	msg = message + (message != "" ? " " : "") + stderrormsg;
}

OOD::NotCompressable::NotCompressable(std::string message)
{
	msg = message;
}

OOD::InvalidObjectArray::InvalidObjectArray(std::string message)
{
	msg = message;
}

OOD::SwitchCaseDefaultReached::SwitchCaseDefaultReached(std::string message)
{
	msg = message;
}

OOD::InvalidArraySize::InvalidArraySize(std::string message)
{
	msg = message;
}

void MemoryDataStream::setDataPtr(char* data)
{
	finish(); // try to deallocate owned data, incase setDataPtr was called after allocation
	baseinit();
	start = current = data;
	readOnly = false;
}

void MemoryDataStream::setDataPtr(const char* data)
{
	finish();
	baseinit();
	start = current = (char*)data;
	readOnly = true;
}


void MemoryDataStream::setMaxSize(size_t size)
{
	firstInvalidAddress = current + size;
}

void MemoryDataStream::setLastValidAddress(const char* lastValidAddr)
{
	firstInvalidAddress = (char*)lastValidAddr + 1;
}

void MemoryDataStream::setFirstInvalidAddress(const char* firstInvalidAddr)
{
	firstInvalidAddress = (char*)firstInvalidAddr;
}

void MemoryDataStream::enableSecurity(bool on)
{
	insecure = !on;
}

void MemoryDataStream::makeReadOnly()
{
	readOnly = true;
}

void MemoryDataStream::setByteEncryptor(std::function<void(char&)> byteEncryptor)
{
	encryptor = byteEncryptor;
	enableBytewiseEncryption = true;
}

void MemoryDataStream::setByteDecryptor(std::function<void(char&)> byteDecryptor)
{
	decryptor = byteDecryptor;
	enableBytewiseEncryption = true;
}

void MemoryDataStream::disableByteEncryption()
{
	encryptor = nullptr;
	decryptor = nullptr;
	enableBytewiseEncryption = false;
}

void MemoryDataStream::setPreprocessor(std::function<void(char*, size_t)> preprocessorFunction)
{
	preprocessor = preprocessorFunction;
	enablePreprocessing = true;
}

void MemoryDataStream::disablePreprocessor()
{
	enablePreprocessing = false;
}

void MemoryDataStream::setPostprocessor(std::function<void(char*, size_t)> postprocessorFunction)
{
	postprocessor = postprocessorFunction;
	enablePostprocessing = true;
}

void MemoryDataStream::disablePostprocessor()
{
	enablePostprocessing = false;
}

void MemoryDataStream::deallocateDataOnDestruction(bool deallocate)
{
	deallocateOnDestruction = deallocate;
}

void MemoryDataStream::allocate(size_t size)
{
	if (selfAllocated)
		delete[] start;
	
	deallocateDataOnDestruction(true);
	selfAllocated = true;

	start = current = new char[size];
	firstInvalidAddress = start + size;

	baseinit();
}

void MemoryDataStream::deallocate()
{
	delete[] start;
	selfAllocated = false;
	start = current = nullptr;
}

MemoryDataStream MemoryDataStream::move()
{
	MemoryDataStream other = *this; // copy everything

	selfAllocated = false;
	deallocateOnDestruction = false;
	start = current = nullptr;
	baseinit();
	return other;
}

void MemoryDataStream::move(MemoryDataStream& mem)
{
	mem = *this; // copy everything

	selfAllocated = false;
	deallocateOnDestruction = false;
	start = current = nullptr;
	baseinit();
}

void MemoryDataStream::finish()
{
	finished = true;
	if (enablePostprocessing)
		postprocessor(start, firstInvalidAddress - start);

	// doesn't makes sense in compination with post processing, but if you have a read only stream, you dont have to deallocate manually
	if (deallocateOnDestruction)
		deallocate();

	if (writeFullStream && current != firstInvalidAddress)
	{
		THROW IncompleteWrite("A MemoryDataStream was deallocated before it was finished.");
	}
}

//this method doesnt mess with the raw data. The NULL terminator isn't included in the std::string. The function never generates zeros
void MemoryDataStream::makeReadable(std::string& str)
{
	//swap some stuff
	for (size_t i = 0; i < str.size() - 1; i += 2)
		std::swap(str[i], str[i + 1]);
	//invert bits
	for (char& c : str)
		if (c != ~0)
			c = ~c;
}

void MemoryDataStream::makeUnreadable(std::string& str)
{
	//invert bits
	for (char& c : str)
		if (c != ~0)
			c = ~c;
	//swap some stuff
	for (size_t i = 0; i < str.size() - 1; i += 2)
		std::swap(str[i], str[i + 1]);
}

void MemoryDataStream::makeReadable(std::wstring& wstr)
{
	//swap some stuff
	for (size_t i = 0; i < wstr.size() - 1; i += 2)
		std::swap(wstr[i], wstr[i + 1]);
	//invert bits
	for (wchar_t& c : wstr)
		if (c != ~0)
			c = ~c;
}

void MemoryDataStream::baseinit()
{
	enableSecurity();
	disableByteEncryption();
	disablePreprocessor();
	disablePostprocessor();
	deallocateDataOnDestruction(false);
	selfAllocated = false;
	finished = false;
}

void MemoryDataStream::makeUnreadable(std::wstring& wstr)
{
	//invert bits
	for (wchar_t& c : wstr)
		if (c != ~0)
			c = ~c;
	//swap some stuff
	for (size_t i = 0; i < wstr.size() - 1; i += 2)
		std::swap(wstr[i], wstr[i + 1]);
}

void MemoryDataStream::write(const char* bytes, size_t size)
{
	if (readOnly)
	{
		THROW ReadOnlyViolation("tried to write to read only location using MemoryDataStream.");
	}

	// validate pointer
	if (current + (size - 1) < firstInvalidAddress || insecure)
	{
		memcpy(current, bytes, size);
		current += size;
	}
	else
	{
		THROW WriteViolation("MemoryDataStream exceeded specified range.");
	}
}

std::string MemoryDataStream::readStr()
{
	std::string result;
	while (peek()) // while the next byte is not a NULL terminator
		result += read();
	// read last null terminator
	read();
	// deobfuscate
	if (obfuscated)
		makeReadable(result);
	return result;
}

std::wstring MemoryDataStream::readWstr()
{
	std::wstring result;
	while (peek<wchar_t>())
		result += read<wchar_t>();
	// read last null terminator
	read<wchar_t>();
	// deobfuscate
	if (obfuscated)
		makeReadable(result);
	return result;
}

void MemoryDataStream::writeStr(std::string str)
{
	// obfuscate
	if (obfuscated)
		makeUnreadable(str);
	write(str.c_str(), str.size() + 1); // 1000 IQ because reusing the null terminator
}

void MemoryDataStream::writeWstr(std::wstring wstr)
{
	// obfuscate
	if (obfuscated)
		makeUnreadable(wstr);
	write((char*)wstr.c_str(), (wstr.size() + 1) * sizeof(wchar_t)); // 1000 IQ because reusing the null terminator
}

unsigned char MemoryDataStream::peek() const
{
	if (current < firstInvalidAddress || insecure)
		return *current;
}

char MemoryDataStream::read()
{
	if (current < firstInvalidAddress || insecure)
		return *current++;
	else
	{
		THROW std::out_of_range("MemoryDataStream exceeded specified range.");
	}
}

void MemoryDataStream::peek(char* destination, size_t size)
{
	if (current + (size - 1) < firstInvalidAddress || insecure)
	{
		memcpy(destination, current, size);
	}
	else
	{
		THROW std::out_of_range("MemoryDataStream exceeded specified range.");
	}
}

void MemoryDataStream::read(char* destination, size_t size)
{
	if (current + (size - 1) < firstInvalidAddress || insecure)
	{
		memcpy(destination, current, size);
		current += size;
	}
	else
	{
		THROW std::out_of_range("MemoryDataStream exceeded specified range.");
	}
}

char* MemoryDataStream::read(size_t size)
{
	if (current + (size - 1) < firstInvalidAddress || insecure)
	{
		char* currentBeforeIncrease = current;
		current += size;
		return currentBeforeIncrease;
	}
	else
	{
		THROW std::out_of_range("MemoryDataStream exceeded specified range.");
	}
}

char* MemoryDataStream::data()
{
	return start;
}

size_t MemoryDataStream::size()
{
	return firstInvalidAddress - start;
}

size_t MemoryDataStream::sizeLeft()
{
	return firstInvalidAddress - current;
}

MemoryDataStream::MemoryDataStream(size_t size)
{
	allocate(size);
}

MemoryDataStream::MemoryDataStream(char* data)
{
	setDataPtr(data);
}

MemoryDataStream::MemoryDataStream(const char* data)
{
	setDataPtr(data);
}

MemoryDataStream::MemoryDataStream(char* data, char* firstInvalidAddr)
{
	setDataPtr(data);
	setFirstInvalidAddress(firstInvalidAddr);
}

MemoryDataStream::MemoryDataStream(const char* data, const char* firstInvalidAddr)
{
	setDataPtr(data);
	setFirstInvalidAddress(firstInvalidAddr);
}

MemoryDataStream::MemoryDataStream(char* data, size_t size)
{
	setDataPtr(data);
	setFirstInvalidAddress(data + size);
}

MemoryDataStream::MemoryDataStream(const char* data, size_t size)
{
	setDataPtr(data);
	setFirstInvalidAddress(data + size);
}

MemoryDataStream::~MemoryDataStream()
{
	if (!finished)
		finish();
}

MemoryDataStream::ReadOnlyViolation::ReadOnlyViolation(std::string message) : runtime_error(message) {}

MemoryDataStream::WriteViolation::WriteViolation(std::string message) : runtime_error(message) {}

MemoryDataStream::IncompleteWrite::IncompleteWrite(std::string message) : runtime_error(message) {}

void OOD::ComplexType::clear()
{
	if (obj)
		delete obj;
	if (size)
		delete size;
}

void OOD::ComplexType::saveToMemory(MemoryDataStream& mem) const
{
	// save type and opt. size specifier
	if (size)
	{
		mem.write<UINT_8>(type + size->sizeSpecifierSpecifier());
		size->save(mem);
	}
	else
		mem.write(type);

	// save object specifier
	if (obj)
	{
		if (FixedObjectSpecifier* fobj = std::get_if<FixedObjectSpecifier>(obj))
			fobj->saveToMemory(mem);
		else if (MixedObjectSpecifier* mobj = std::get_if<MixedObjectSpecifier>(obj))
			mobj->saveToMemory(mem);
	}
}

void OOD::ComplexType::setType(TypeSpecifier type)
{
	//things to do:
	// clear object
	// set type
	// if obj specifier needed, select fixed or mixed variant
	// construct size specifier if needed
	clear();
	this->type = type;
	if (type == Type::MXOBJ)
		obj = new ObjectSpecifier(MixedObjectSpecifier());
	else if (type == Type::FXOBJ)
		obj = new ObjectSpecifier(FixedObjectSpecifier());

	if (type == Type::MXLST || type == Type::FXLST)
		size = new SizeSpecifier;
}

bool OOD::ComplexType::isPrimitive() const
{
	// this can be easily determined, by checking if the object has any additional specifier
	return !(obj && size);
}

bool OOD::ComplexType::isString() const
{
	return type != Type::CSTR && type != Type::WSTR;
}

Type& ODF::Type::operator=(const Type& other)
{
	if (&other == *this)
		return *this;

	if (other.obj)
		obj = new ObjectSpecifier(*other.obj);

	if (other.size)
		size = new SizeSpecifier(*other.size);

	type = other.type;

	return *this;
}

void OOD::MixedObjectSpecifier::saveToMemory(MemoryDataStream& mem) const
{
	// see documentation "Mixed Object Specifier"
	for (const auto& pair_it : properties)
	{
		// save key
		mem.writeStr(pair_it.first);
		// save type
		pair_it.second.saveToMemory(mem);
	}
	// save zero to end object specifier
	mem.write<UINT_8>(0);
}

void OOD::FixedObjectSpecifier::saveToMemory(MemoryDataStream& mem) const
{
	// save type
	header.saveToMemory(mem);
	// save keys
	for (const std::string& key : keys)
		mem.writeStr(key);
	// save zero to end specifier
	mem.write<UINT_8>(0);
}

OOD::ComplexType::ComplexType()
{
	obj = nullptr;
	size = nullptr;
}

OOD::ComplexType::~ComplexType()
{
}

OOD::TypeSpecifier OOD::SizeSpecifier::sizeSpecifierSpecifier() const
{
	if (actualSize < OverflowSize8bit)
		return 0b00;
	else if (actualSize < OverflowSize16bit)
		return 0b01;
	else if (actualSize < OverflowSize32bit)
		return 0b10;
	else
		return 0b11;
}

void OOD::SizeSpecifier::load(MemoryDataStream& stream, TypeSpecifier type)
{
	switch (type % 4)
	{
	case 0b00:
		actualSize = stream.read<UINT_8>();
		return;
	case 0b01:
		actualSize = stream.read<UINT_16>();
		return;
	case 0b10:
		actualSize = stream.read<UINT_32>();
		return;
	case 0b11:
		actualSize = stream.read<UINT_64>();
		return;
	}
}

void OOD::SizeSpecifier::save(MemoryDataStream& stream, TypeSpecifier type) const
{
	switch (type % 4)
	{
	case 0b00:
		stream.write((const char*)&actualSize, 1);
		return;
	case 0b01:
		stream.write((const char*)&actualSize, 2);
		return;
	case 0b10:
		stream.write((const char*)&actualSize, 4);
		return;
	case 0b11:
		stream.write((const char*)&actualSize, 8);
		return;
	}
}

void OOD::SizeSpecifier::save(MemoryDataStream& stream) const
{
	save(stream, sizeSpecifierSpecifier());
}
