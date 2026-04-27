#include "ODF.h"

bool ODF::Config::PreventAutomaticMergeLossFloat = true;
bool ODF::Config::PreventAutomaticMergeLossString = true;
bool ODF::printType = true;
#ifdef ODF_THREADSAFE
std::mutex ODF::printMutex;
#endif
unsigned char ODF::Config::DefaultIntegerMergeSize = 0;
ODF::ComplexExplicitor::OBJSTRUCT ODF::__MOBJ{};
ODF::ComplexExplicitor::FOBJSTRUCT ODF::__FOBJ{};

// little istream helper
std::vector<char> read_all_bytes(std::istream& input) {
	std::vector<char> buffer;

	// If it's a file, we can pre-allocate
	// (This works even if 'input' is passed as a generic istream)
	input.seekg(0, std::ios::end);
	auto size = input.tellg();
	if (size > 0) {
		input.seekg(0, std::ios::beg);
		buffer.resize(static_cast<size_t>(size));
		input.read(buffer.data(), size);
	}
	else {
		// Fallback for non-seekable streams (like cin)
		buffer.assign(std::istreambuf_iterator<char>(input),
			std::istreambuf_iterator<char>());
	}

	return buffer; // No std::move needed!
}

#pragma region specifiers
void ODF::Type::makeImmutable()
{
#ifdef _DEBUG
	if (immutable)
		std::cout << "Warning: ODF::Type::makeImmutable() called although object is already immutable\n";
#endif
	immutable = true;
}

void ODF::Type::checkImmutable() const
{
	if (immutable && type) // ignore this if type is NULL
	{
		THROW InvalidTypeMutation();
	}
}

void ODF::Type::readTypeSpecifier(MemoryDataStream& mem)
{
	checkImmutable();
	*this = mem.read<unsigned char>();
}

ODF::Status ODF::Type::saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const
{
	// check if the type is registered in the parse Info. and use USETYPE in that case
	if (!(parseInfo.flags & ConstParseInfo::FLAG_NO_USETYPE)) // skip if FLAG_NO_USETYPE is set
	{
		parseInfo.vtypeinfo->useType(mem, *this, parseInfo);
		return Status::Ok;
	}

	// specifier priorities:
	// 1. Type
	// 2. Size
	// 3. Fixtype
	// 4. Object

	// implemented priorities:
	// type (byte)
	// array specifier (size + fixtype)
	// ObjectSpecifier

	// save primitive type
	mem.write<unsigned char>(type.byte()); // Prio 1
	bool obj = type.isObject();
	bool list = type.isList();
	bool fixed = type.isFixed();
	bool mixed = type.isMixed();

	if (!obj && !list)
		return Status::Ok;

	if (list)
	{
		ArraySpecifier& listSpec = std::get<ArraySpecifier>(*complexSpec);
		if (fixed)
		{
			ConstParseInfo localParseInfo = parseInfo;
			localParseInfo.flags &= ~ConstParseInfo::FLAG_NO_USETYPE; // allow usetypes, as the current type can no longer try to use itself and the current type can use USETYPEs for subtypes for memory efficiency.
			localParseInfo.flags |= ConstParseInfo::FLAG_NO_IMPLICIT_DEFTYPE; // don't allow deftypes, as the program is currently in a specifier which shouldn't be interrupted by for example DEFTYPEs
			FixedArraySpecifier& flistSpec = std::get<FixedArraySpecifier>(listSpec);
			flistSpec.saveToMemory(mem, localParseInfo);
			return Status::Ok;
		}
		else if (mixed)
		{
			ConstParseInfo localParseInfo = parseInfo;
			localParseInfo.flags &= ~ConstParseInfo::FLAG_NO_USETYPE; // allow usetypes, as the current type can no longer try to use itself and the current type can use USETYPEs for subtypes for memory efficiency.
			localParseInfo.flags |= ConstParseInfo::FLAG_NO_IMPLICIT_DEFTYPE; // don't allow deftypes, as the program is currently in a specifier which shouldn't be interrupted by for example DEFTYPEs
			MixedArraySpecifier& mlistSpec = std::get<MixedArraySpecifier>(listSpec);
			mlistSpec.saveToMemory(mem, localParseInfo);
			return Status::Ok;
		}
	}
	else if (obj)
	{
		ObjectSpecifier& objSpec = std::get<ObjectSpecifier>(*complexSpec);
		if (fixed)
		{
			ConstParseInfo localParseInfo = parseInfo;
			localParseInfo.flags &= ~ConstParseInfo::FLAG_NO_USETYPE; // allow usetypes, as the current type can no longer try to use itself and the current type can use USETYPEs for subtypes for memory efficiency.
			localParseInfo.flags |= ConstParseInfo::FLAG_NO_IMPLICIT_DEFTYPE; // don't allow deftypes, as the program is currently in a specifier which shouldn't be interrupted by for example DEFTYPEs
			FixedObjectSpecifier& fobjSpec = std::get<FixedObjectSpecifier>(objSpec);
			fobjSpec.saveToMemory(mem, localParseInfo);
			return Status::Ok;
		}
		else if (mixed)
		{
			ConstParseInfo localParseInfo = parseInfo;
			localParseInfo.flags &= ~ConstParseInfo::FLAG_NO_USETYPE; // allow usetypes, as the current type can no longer try to use itself and the current type can use USETYPEs for subtypes for memory efficiency.
			localParseInfo.flags |= ConstParseInfo::FLAG_NO_IMPLICIT_DEFTYPE; // don't allow deftypes, as the program is currently in a specifier which shouldn't be interrupted by for example DEFTYPEs
			MixedObjectSpecifier& mobjSpec = std::get<MixedObjectSpecifier>(objSpec);
			mobjSpec.saveToMemory(mem, localParseInfo);
			return Status::Ok;
		}
	}

	return Status::Ok;
}

ODF::Status ODF::Type::loadFromMemory(MemoryDataStream& mem, const ParseInfo& parseInfo)
{
	// makes sure every pointer is valid
	if (parseInfo.containsInvalidPointer())
	{
		THROW InvalidParseInformation();
	}

	destroyComplexSpec(); // destroy the last complexSpec if the object was already in use

	// check for virtual types
	unsigned char vtype = mem.peek();
	if (TypeSpecifier::isVirtual(vtype))
	{
		// the local pool will be used to parse the virtual type. If existing, the provided PoolCollection will also be used, though one will be created in any scenario and left empty in case the pools optional was empty

		// it is required that lPool points to a valid pool. If no localPool is provided, one will be created. Same for the poolCollection
		try // parse(...) might throw
		{
			std::optional<std::pair<ODF::Type, ODF::VTypeInfo::TypeID>> optType = parseInfo.pools->parse(mem, parseInfo.localPool, parseInfo.vtypeinfo);
			if (optType.has_value())
			{
				*this = optType.value().first;
				if (parseInfo.dependecies)
					parseInfo.dependecies->push_back(optType.value().second);
				return Status::Ok;
			}
		}
		catch (Status& error)
		{
			return error;
		}
		return Status::Virtual;
	}

	// load primitive type specifier, to allow isFixed() check
	// else readTypeSpecifier(mem); // prio 1
	else
	{
		checkImmutable();
		*this = mem.read<unsigned char>();
	}
	
	bool obj = type.isObject();
	bool list = type.isList();
	bool fixed = type.isFixed();
	bool mixed = type.isMixed();

	if (!obj && !list)
		return Status::Ok;

	makeComplexSpec(); // make sure there's a valid complexSpec

	// load individual complex types
	if (list)
	{
		ArraySpecifier& listSpec = complexSpec->emplace<ArraySpecifier>();
		if (fixed)
		{
			FixedArraySpecifier& flistSpec = listSpec.emplace<FixedArraySpecifier>();
			flistSpec.loadFromMemory(mem, parseInfo);
			return Status::Ok;
		}
		else if (mixed)
		{
			MixedArraySpecifier& mlistSpec = listSpec.emplace<MixedArraySpecifier>();
			mlistSpec.loadFromMemory(mem, parseInfo);
			return Status::Ok;
		}
	}
	else if (obj)
	{
		ObjectSpecifier& objSpec = complexSpec->emplace<ObjectSpecifier>();
		if (fixed)
		{
			FixedObjectSpecifier& fobjSpec = objSpec.emplace<FixedObjectSpecifier>();
			fobjSpec.loadFromMemory(mem, parseInfo); // already handles all loading
			return Status::Ok;
		}
		else if (mixed)
		{
			MixedObjectSpecifier& mobjSpec = objSpec.emplace<MixedObjectSpecifier>();
			mobjSpec.loadFromMemory(mem, parseInfo);
			return Status::Ok;
		}
	}

	return Status::Ok;
}

#pragma region TypeSpecifier
unsigned char ODF::TypeSpecifier::byte() const
{
	return type;
}

ODF::TypeSpecifier::Type ODF::TypeSpecifier::sizeSpec() const
{
	return type & 0b11000000;
}

bool ODF::TypeSpecifier::isSigned() const
{
	return !(type & 0b00100000);
}

bool ODF::TypeSpecifier::isUnsigned() const
{
	return type & 0b00100000;
}

bool ODF::TypeSpecifier::isWidestring()
{
	return type & 0b00100000;
}

ODF::TypeSpecifier::operator Type() const
{
	// without size spec
	return type;
}

ODF::TypeSpecifier& ODF::TypeSpecifier::operator=(unsigned char byte)
{
	type = byte;
	return *this;
}

ODF::TypeSpecifier::TypeSpecifier(Type t)
{
	type = t;
}

ODF::TypeSpecifier operator<<(ODF::TypeSpecifier spec, unsigned char shiftBy)
{
	return spec.type << shiftBy;
}

ODF::TypeSpecifier operator>>(ODF::TypeSpecifier spec, unsigned char shiftBy)
{
	return spec.type >> shiftBy;
}

ODF::TypeSpecifier operator&(ODF::TypeSpecifier spec, unsigned char byte)
{
	return spec.type & byte;
}

ODF::TypeSpecifier operator|(ODF::TypeSpecifier spec, unsigned char byte)
{
	return spec.type | byte;
}

bool operator==(ODF::TypeSpecifier spec1, ODF::TypeSpecifier spec2)
{
	return spec1.byte() == spec2.byte();
}

bool operator!=(ODF::TypeSpecifier spec1, ODF::TypeSpecifier spec2)
{
	return spec1.byte() != spec2.byte();
}

bool operator==(ODF::TypeSpecifier spec, ODF::TypeSpecifier::Type type)
{
	return spec.byte() == type;
}

bool operator!=(ODF::TypeSpecifier spec, ODF::TypeSpecifier::Type type)
{
	return spec.byte() != type;
}

bool operator!=(const ODF::Type& t1, const ODF::Type& t2)
{
	return !(t1 == t2);
}

bool DF_API operator==(const ODF::Object& obj1, const ODF::Object& obj2)
{
	if (obj1.size() != obj2.size())
		return false;

	if (obj1.isMixed() != obj2.isMixed())
		return false;

	try {
		for (const ODF::Pair& it : obj1)
			if (obj2.at(it.first) != it.second)
				return false;
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool DF_API operator!=(const ODF::Object& obj1, const ODF::Object& obj2)
{
	return !(obj1 == obj2);
}

bool operator==(const ODF::List& arr1, const ODF::List& arr2)
{
	if (arr1.size() != arr2.size())
		return false;

	if (arr1.isMixed() != arr2.isMixed())
		return false;

	for (size_t i = 0; i < arr1.size(); i++)
		if (arr1[i] != arr2[i])
			return false;
	return true;
}

bool operator!=(const ODF::List& arr1, const ODF::List& arr2)
{
	return !operator==(arr1, arr2);
}

std::ostream& operator<<(std::ostream& out, const ODF& odf)
{
	return ODF::print(out, odf, ODF::PrettyPrintInfo());
}

std::ostream& operator<<(std::ostream& out, const ODF::TypeSpecifier& tspec)
{
	using TS = ODF::TypeSpecifier;
	switch (tspec.withoutSSS())
	{
	case TS::NULLTYPE: return out << "NULLTYPE";
	case TS::UNDEFINED: return out << "UNDEFINED";
	case TS::BYTE: return out << "BYTE";
	case TS::UBYTE: return out << "UBYTE";
	case TS::SHORT: return out << "SHORT";
	case TS::USHORT: return out << "USHORT";
	case TS::INT: return out << "INT";
	case TS::UINT: return out << "UINT";
	case TS::LONG: return out << "LONG";
	case TS::ULONG: return out << "ULONG";
	case TS::FLOAT: return out << "FLOAT";
	case TS::DOUBLE: return out << "DOUBLE";
	case TS::CSTR: return out << "CSTR";
	case TS::WSTR: return out << "WSTR";
	case TS::FXOBJ: return out << "FXOBJ";
	case TS::MXOBJ: return out << "MXOBJ";
	case TS::FXLIST: return out << "FXLIST";
	case TS::MXLIST: return out << "MXLIST";
	case TS::DEFTYPE: return out << "DEFTYPE";
	case TS::USETYPE: return out << "USETYPE";
	case TS::EXPORT: return out << "EXPORT";
	case TS::EXPORTAS: return out << "EXPORT";
	case TS::IMPORT: return out << "IMPORT";
	case TS::IMPORTAS: return out << "IMPORT";
	default: return out << "TYPE ERROR";
	}
}

std::ostream& operator<<(std::ostream& out, const ODF::Object& obj)
{
	if (obj.isMixed())
	{
		out << "<MX>\n";
		if (!obj.size())
		{
			out << "EMPTY\n";
			return out;
		}
		for (const ODF::Pair& it : obj)
			out << "[\"" << it.first << "\"]:\t" << it.second << "\n";
	}
	else
	{
		out << "<FX(" << obj.getFixedDatatype() << ")>\n";
		if (!obj.size())
		{
			out << "EMPTY\n";
			return out;
		}

		ODF_THREADSAFE_PRINT;
		ODF_DISABLE_TYPE_PRINT;

		//
		// TODO
		// The error in the for loop below is that ODF::Object also inherits the AbstractObject::map, although it should only be present in the Fixed and Mixed Object Implemetatio.
		// But with this error, the for loop iterates over the Object::map and not Object::AbstractObject map, which is empty because never accessed
		//

		for (const ODF::Pair& it : obj)
		{
			out << "[\"" << it.first << "\"]:\t" << it.second << "\n";
		}

		ODF_RESTORE_TYPE_PRINT;
	}
	return out;
}

std::ostream& operator<<(std::ostream& out, const ODF::List& list)
{
	if (list.isMixed())
	{
		out << "<MX>\n";
		if (!list.size())
		{
			out << "EMPTY\n";
			return out;
		}
		size_t i = 0;
		for (const ODF& odf : const_cast<ODF::List&>(list)) // bruh
			out << i++ << ": " << odf << "\n";
	}
	else
	{
		out << "<FX(" << list.getFixedDatatype() << ")>\n";
		if (!list.size())
		{
			out << "EMPTY\n";
			return out;
		}

		ODF_THREADSAFE_PRINT;
		ODF_DISABLE_TYPE_PRINT;

		size_t i = 0;
		for (const ODF& odf : list) // bruh
			out << i++ << ": " << odf << "\n";

		ODF_RESTORE_TYPE_PRINT;
	}

	return out;
}

std::ostream& operator<<(std::ostream& out, const std::wstring& wstr)
{
	for (wchar_t wc : wstr) out << static_cast<char>(wc);
	return out;
}

std::ostream& operator<<(std::ostream& out, const ODF::Type& type)
{
	using TS = ODF::TypeSpecifier;
	switch (type.getswitch())
	{
	case TS::BYTE: out << "BYTE"; return out;
	case TS::UBYTE: out << "UBYTE"; return out;
	case TS::SHORT: out << "UBYTE"; return out;
	case TS::USHORT: out << "USHORT"; return out;
	case TS::INT: out << "INT"; return out;
	case TS::UINT: out << "UINT"; return out;
	case TS::LONG: out << "LONG"; return out;
	case TS::ULONG: out << "ULONG"; return out;
	case TS::FLOAT: out << "FLOAT"; return out;
	case TS::DOUBLE: out << "DOUBLE"; return out;
	case TS::CSTR: out << "CSTR"; return out;
	case TS::WSTR: out << "WSTR"; return out;
	//case TS::FXOBJ: out << "FXOBJ(" << std::get<ODF::FixedObjectSpecifier>(*type.obj).header << ")"; return out;
	case TS::MXOBJ: out << "MXOBJ"; return out;
	case TS::FXLIST: out << "FXLST"; return out;
	}
	return out;
}

std::ostream& ODF::print(std::ostream& out, const ODF& odf, ODF::PrettyPrintInfo ppi)
{
	if (auto ptr = std::get_if<INT_8>(&odf.content)) { out << (odf.printType ? "[BYTE] " : "") << (signed short)*ptr; }
	if (auto ptr = std::get_if<UINT_8>(&odf.content)) { out << (odf.printType ? "[UBYTE] " : "") << (signed short)*ptr; }
	if (auto ptr = std::get_if<INT_16>(&odf.content)) { out << (odf.printType ? "[SHORT] " : "") << *ptr; }
	if (auto ptr = std::get_if<UINT_16>(&odf.content)) { out << (odf.printType ? "[USHORT] " : "") << *ptr; }
	if (auto ptr = std::get_if<INT_32>(&odf.content)) { out << (odf.printType ? "[INT] " : "") << *ptr; }
	if (auto ptr = std::get_if<UINT_32>(&odf.content)) { out << (odf.printType ? "[UINT] " : "") << *ptr; }
	if (auto ptr = std::get_if<INT_64>(&odf.content)) { out << (odf.printType ? "[LONG] " : "") << *ptr; }
	if (auto ptr = std::get_if<UINT_64>(&odf.content)) { out << (odf.printType ? "[ULONG] " : "") << *ptr; }
	if (auto ptr = std::get_if<float>(&odf.content)) { out << (odf.printType ? "[FLOAT] " : "") << *ptr; }
	if (auto ptr = std::get_if<double>(&odf.content)) { out << (odf.printType ? "[DOUBLE] " : "") << *ptr; }
	if (auto ptr = std::get_if<std::string>(&odf.content)) { out << (odf.printType ? "[CSTR] " : "") << *ptr; }
	if (auto ptr = std::get_if<std::wstring>(&odf.content)) { out << (odf.printType ? "[WSTR] " : "") << *ptr; }
	if (auto ptr = std::get_if<ODF::Object>(&odf.content)) { print(out << (odf.printType ? "[VT_OBJ] " : ""), *ptr, ppi); }
	if (auto ptr = std::get_if<ODF::List>(&odf.content)) { print(out << (odf.printType ? "[VT_LIST] " : ""), *ptr, ppi); }
	return out;
}

std::ostream& ODF::print(std::ostream& out, const ODF::Object& obj, ODF::PrettyPrintInfo ppi)
{
	if (obj.isMixed())
	{
		out << "<MX>\n";
		if (!obj.size())
		{
			out << "EMPTY\n";
			return out;
		}
		ppi.indent++;
		for (size_t i = 0; const ODF::Pair& it : obj)
		{
			ppi.pre(out);
			print(out << "[\"" << it.first << "\"]:  ", it.second, ppi);
			if (i != obj.size() - 1)
				out << "\n";
			ppi.post(out);
			i++;
		}
		ppi.indent--;
		ppi.post(out);
	}
	else
	{
		out << "<FX(" << obj.getFixedDatatype() << ")>\n";
		if (!obj.size())
		{
			out << "EMPTY\n";
			return out;
		}
		ppi.post(out);

		ODF_THREADSAFE_PRINT;
		ODF_DISABLE_TYPE_PRINT;

		//
		// TODO
		// The error in the for loop below is that ODF::Object also inherits the AbstractObject::map, although it should only be present in the Fixed and Mixed Object Implemetatio.
		// But with this error, the for loop iterates over the Object::map and not Object::AbstractObject map, which is empty because never accessed
		//

		ppi.indent++;
		for (size_t i = 0; const ODF::Pair& it : obj)
		{
			ppi.pre(out);
			print(out << "[\"" << it.first << "\"]:  ", it.second, ppi);
			if (i != obj.size() - 1)
				out << "\n";
			ppi.post(out);
			i++;
		}
		ppi.indent--;

		ODF_RESTORE_TYPE_PRINT;
	}
	return out;
}

std::ostream& ODF::print(std::ostream& out, const ODF::List& list, ODF::PrettyPrintInfo ppi)
{
	ppi.pre(out);
	if (list.isMixed())
	{
		out << "<MX>\n";
		if (!list.size())
		{
			out << "EMPTY\n";
			return out;
		}
		size_t i = 0;
		ppi.indent++;
		for (size_t i = 0;  const ODF& odf : const_cast<ODF::List&>(list)) // bruh
		{
			ppi.pre(out);
			print(out << i++ << ": ", odf, ppi);
			if (i != list.size() - 1)
				out << "\n";
			ppi.post(out);
			i++;
		}
		ppi.indent--;
	}
	else
	{
		out << "<FX(" << list.getFixedDatatype() << ")>\n";
		if (!list.size())
		{
			out << "EMPTY\n";
			return out;
		}

		ODF_THREADSAFE_PRINT;
		ODF_DISABLE_TYPE_PRINT;

		size_t i = 0;
		ppi.indent++;
		for (size_t i = 0; const ODF& odf : list) // bruh
		{
			ppi.pre(out);
			print(out << i++ << ": ", odf, ppi);
			if (i != list.size() - 1)
				out << "\n";
			ppi.post(out);
		}
		ppi.indent--;

		ODF_RESTORE_TYPE_PRINT;
	}

	ppi.post(out);
	return out;
}

std::ostream& ODF::print(std::ostream& out, const ODF::Type& type, ODF::PrettyPrintInfo ppi)
{
	using TS = ODF::TypeSpecifier;
	switch (type.getswitch())
	{
	case TS::BYTE: out << "BYTE"; return out;
	case TS::UBYTE: out << "UBYTE"; return out;
	case TS::SHORT: out << "UBYTE"; return out;
	case TS::USHORT: out << "USHORT"; return out;
	case TS::INT: out << "INT"; return out;
	case TS::UINT: out << "UINT"; return out;
	case TS::LONG: out << "LONG"; return out;
	case TS::ULONG: out << "ULONG"; return out;
	case TS::FLOAT: out << "FLOAT"; return out;
	case TS::DOUBLE: out << "DOUBLE"; return out;
	case TS::CSTR: out << "CSTR"; return out;
	case TS::WSTR: out << "WSTR"; return out;
	case TS::FXOBJ: out << "FXOBJ(" << std::get<ODF::FixedObjectSpecifier>(std::get<ODF::ObjectSpecifier>(*type.complexSpec)).fixType << ")"; return out;
	case TS::MXOBJ: out << "MXOBJ"; return out;
	case TS::FXLIST: out << "FXLST(" << std::get<ODF::FixedArraySpecifier>(std::get<ODF::ArraySpecifier>(*type.complexSpec)).fixType << ")"; return out;
	case TS::MXLIST: out << "MXLST"; return out;
	}
	return out;
}

bool operator==(const ODF& odf1, const ODF& odf2)
{
	if (odf1.type != odf2.type) // check type, size and content headers
		return false;

 	switch (odf1.getVariantType())
	{
	case ODF::VT_INT8: return std::get<INT_8>(odf1.content) == std::get<INT_8>(odf2.content);
	case ODF::VT_UINT8: return std::get<UINT_8>(odf1.content) == std::get<UINT_8>(odf2.content);
	case ODF::VT_INT16: return std::get<INT_16>(odf1.content) == std::get<INT_16>(odf2.content);
	case ODF::VT_UINT16: return std::get<UINT_16>(odf1.content) == std::get<UINT_16>(odf2.content);
	case ODF::VT_INT32: return std::get<INT_32>(odf1.content) == std::get<INT_32>(odf2.content);
	case ODF::VT_UINT32: return std::get<UINT_32>(odf1.content) == std::get<UINT_32>(odf2.content);
	case ODF::VT_INT64: return std::get<INT_64>(odf1.content) == std::get<INT_64>(odf2.content);
	case ODF::VT_UINT64: return std::get<UINT_64>(odf1.content) == std::get<UINT_64>(odf2.content);
	case ODF::VT_FLOAT: return std::get<float>(odf1.content) == std::get<float>(odf2.content);
	case ODF::VT_DOUBLE: return std::get<double>(odf1.content) == std::get<double>(odf2.content);
	case ODF::VT_CSTR: return std::get<std::string>(odf1.content) == std::get<std::string>(odf2.content);
	case ODF::VT_WSTR: return std::get<std::wstring>(odf1.content) == std::get<std::wstring>(odf2.content);
	case ODF::VT_LIST: return std::get<ODF::List>(odf1.content) == std::get<ODF::List>(odf2.content);
	case ODF::VT_OBJ: return std::get<ODF::Object>(odf1.content) == std::get<ODF::Object>(odf2.content);
	}
}

bool ODF::Type::isString() const
{
	return type & TypeSpecifier::FLAG_STRING;
}

inline ODF::TypeSpecifier ODF::Type::getswitch() const
{
	return type;
}

inline ODF::VariantType ODF::Type::getVariantType() const
{
	using TS = TypeSpecifier;
	switch (type.withoutSSS())
	{
	case TS::BYTE: return VT_INT8;
	case TS::UBYTE: return VT_UINT8;
	case TS::SHORT: return VT_INT16;
	case TS::USHORT: return VT_UINT16;
	case TS::INT: return VT_INT32;
	case TS::UINT: return VT_UINT32;
	case TS::LONG: return VT_INT64;
	case TS::ULONG: return VT_UINT64;
	case TS::FLOAT: return VT_FLOAT;
	case TS::DOUBLE: return VT_DOUBLE;
	case TS::CSTR: return VT_CSTR;
	case TS::WSTR: return VT_WSTR;
	case TS::FXLIST: case TS::MXLIST: return VT_LIST;
	case TS::FXOBJ: case TS::MXOBJ: return VT_OBJ;
	default: THROW VariantTypeConversionError();
	}
}

ODF::SizeSpecifier* ODF::Type::sizeSpecifier() const
{
	if (!complexSpec)
		return nullptr;
	if (ArraySpecifier* arrSpec = std::get_if<ArraySpecifier>(complexSpec))
		if (FixedArraySpecifier* farrSpec = std::get_if<FixedArraySpecifier>(arrSpec))
			return &farrSpec->size;
	return nullptr;
}

ODF::ArraySpecifier* ODF::Type::arraySpecifier() const
{
	if (!complexSpec)
		return nullptr;
	if (ArraySpecifier* arrSpec = std::get_if<ArraySpecifier>(complexSpec))
		return arrSpec;
	return nullptr;
}

ODF::FixedArraySpecifier* ODF::Type::fixedArraySpecifier() const
{
	if (!complexSpec)
		return nullptr;
	if (ArraySpecifier* arrSpec = std::get_if<ArraySpecifier>(complexSpec))
		if (FixedArraySpecifier* farrSpec = std::get_if<FixedArraySpecifier>(arrSpec))
			return farrSpec;
	return nullptr;
}

ODF::MixedArraySpecifier* ODF::Type::mixedArraySpecifier() const
{
	if (!complexSpec)
		return nullptr;
	if (ArraySpecifier* arrSpec = std::get_if<ArraySpecifier>(complexSpec))
		if (MixedArraySpecifier* marrSpec = std::get_if<MixedArraySpecifier>(arrSpec))
			return marrSpec;
	return nullptr;
}

ODF::ObjectSpecifier* ODF::Type::objectSpecifier() const
{
	if (!complexSpec)
		return nullptr;
	if (ObjectSpecifier* objSpec = std::get_if<ObjectSpecifier>(complexSpec))
		return objSpec;
	return nullptr;
}

ODF::FixedObjectSpecifier* ODF::Type::fixedObjectSpecifier() const
{
	if (!complexSpec)
		return nullptr;
	if (ObjectSpecifier* objSpec = std::get_if<ObjectSpecifier>(complexSpec))
		if (FixedObjectSpecifier* fobjSpec = std::get_if<FixedObjectSpecifier>(objSpec))
			return fobjSpec;
	return nullptr;
}

ODF::MixedObjectSpecifier* ODF::Type::mixedObjectSpecifier() const
{
	if (!complexSpec)
		return nullptr;
	if (ObjectSpecifier* objSpec = std::get_if<ObjectSpecifier>(complexSpec))
		if (MixedObjectSpecifier* mobjSpec = std::get_if<MixedObjectSpecifier>(objSpec))
			return mobjSpec;
	return nullptr;
}

bool ODF::Type::isConvertableTo(const Type& other) const
{
	if (other.getTypeClass() != getTypeClass())
		return false;
	if (getTypeClass() != TypeClass::Int && getTypeClass() != TypeClass::Float)
		return false;
	if (getSize() > other.getSize())
		return false;
	if (getSize() < other.getSize())
		return true;
	return isUnsigned() == other.isUnsigned();
}

bool ODF::Type::isUnsigned() const
{
	using TS = TypeSpecifier;
	switch (type)
	{
	case TS::UBYTE:
	case TS::USHORT:
	case TS::UINT:
	case TS::ULONG:
		return true;
	default:
		return false;
	}
}

unsigned char ODF::Type::getSize() const
{
	using TS = TypeSpecifier;
	switch (type)
	{
	case TS::BYTE: return sizeof(INT_8);
	case TS::UBYTE: return sizeof(UINT_8);
	case TS::SHORT: return sizeof(INT_16);
	case TS::USHORT: return sizeof(UINT_16);
	case TS::INT: return sizeof(INT_32);
	case TS::UINT: return sizeof(UINT_32);
	case TS::LONG: return sizeof(INT_64);
	case TS::ULONG: return sizeof(UINT_64);
	case TS::FLOAT: return sizeof(float);
	case TS::DOUBLE: return sizeof(double);
	case TS::CSTR: return 1;
	case TS::WSTR: return 2;
	default: return 3;
	}
}

void ODF::Type::setFixType(const Type& type)
{
	const Type* ftype = getFixType();
	if (ftype)
		if (*ftype == type)
			return; // fixtype already same

	checkImmutable();
	if (isPrimitive() || isMixed())
	{
		THROW InvalidType();
	}

	if (isList())
		std::get<FixedArraySpecifier>(std::get<ArraySpecifier>(*complexSpec)).fixType = type;
	else
		std::get<FixedObjectSpecifier>(std::get<ObjectSpecifier>(*complexSpec)).fixType = type;
}

const ODF::Type* ODF::Type::getFixType() const
{
	if (isPrimitive() || isMixed())
		return nullptr;

	if (isList())
		return &std::get<FixedArraySpecifier>(std::get<ArraySpecifier>(*complexSpec)).fixType;
	else
		return &std::get<FixedObjectSpecifier>(std::get<ObjectSpecifier>(*complexSpec)).fixType;
}

ODF::TypeClass ODF::Type::getTypeClass() const
{
	using TS = TypeSpecifier;
	switch (type.withoutSSS())
	{
	case TS::BYTE:
	case TS::UBYTE:
	case TS::SHORT:
	case TS::USHORT:
	case TS::INT:
	case TS::UINT:
	case TS::LONG:
	case TS::ULONG:
		return TypeClass::Int;
	case TS::FLOAT:
	case TS::DOUBLE:
		return TypeClass::Float;
	case TS::CSTR:
	case TS::WSTR:
		return TypeClass::String;
	case TS::FXLIST:
	case TS::MXLIST:
		return TypeClass::List;
	case TS::FXOBJ:
	case TS::MXOBJ:
		return TypeClass::Object;
	default:
		THROW InvalidCondition();
	}
}

const std::vector<ODF::Type>* ODF::Type::getArrayTypes() const
{
	if (!isMixed() || !isList())
		return nullptr;
	return &std::get<MixedArraySpecifier>(std::get<ArraySpecifier>(*complexSpec)).types;
}

const std::unordered_map<std::string, ODF::Type>* ODF::Type::getObjectTypes() const
{
	if (!isMixed() || !isObject())
		return nullptr;
	return &std::get<MixedObjectSpecifier>(std::get<ObjectSpecifier>(*complexSpec)).properties;
}

void ODF::Type::setSSS(unsigned char sss)
{
	checkImmutable();
	type = (unsigned char)((type.byte() & 0b00'111111) | sss);
}

void ODF::Type::destroyComplexSpec()
{
	checkImmutable();
	if (complexSpec)
		delete complexSpec;
	complexSpec = nullptr;
}

void ODF::Type::makeComplexSpec()
{
	checkImmutable(); // only here for clarity, technically not needed because the type remains unchanged
	if (!complexSpec)
		complexSpec = new ComplexSpecifier;
}

void ODF::Type::resetComplexSpec()
{
	checkImmutable();
	destroyComplexSpec();
	complexSpec = new ComplexSpecifier;
}

ODF::VariantType ODF::getVariantType() const
{
	return type.getVariantType();
}

ODF::Type ODF::getComplexType() const
{
	return type;
}

ODF::TypeSpecifier ODF::getPrimitiveType() const
{
	return TypeSpecifier();
}

ODF::TypeClass ODF::getTypeClass() const
{
	return TypeClass();
}

unsigned char ODF::getPrecision() const
{
	return 0;
}

ODF::TypeSpecifier ODF::getTypeFromTC(TypeClass tc, unsigned char precision)
{
	return TypeSpecifier();
}

ODF& ODF::operator[](size_t index)
{
	return std::get<List>(content)[index];
}

ODF& ODF::operator[](std::string_view key)
{
	if (auto ptr = std::get_if<Object>(&content))
		return ptr->operator[](key.data());
	makeObject();
	return std::get<Object>(content)[key.data()];
}

ODF& ODF::operator=(const ODF& other)
{
	if (&other == this)
		return *this;

	type = other.type;
	content = other.content;
	return *this;
}

ODF& ODF::operator=(INT_8 val)
{
	type = TypeSpecifier::BYTE;
	content = val;
	return *this;
}

ODF& ODF::operator=(UINT_8 val)
{
	type = TypeSpecifier::UBYTE;
	content = val;
	return *this;
}

ODF& ODF::operator=(INT_16 val)
{
	type = TypeSpecifier::SHORT;
	content = val;
	return *this;
}

ODF& ODF::operator=(UINT_16 val)
{
	type = TypeSpecifier::USHORT;
	content = val;
	return *this;
}

ODF& ODF::operator=(INT_32 val)
{
	type = TypeSpecifier::INT;
	content = val;
	return *this;
}

ODF& ODF::operator=(UINT_32 val)
{
	type = TypeSpecifier::UINT;
	content = val;
	return *this;
}

ODF& ODF::operator=(INT_64 val)
{
	type = TypeSpecifier::LONG;
	content = val;
	return *this;
}

ODF& ODF::operator=(UINT_64 val)
{
	type = TypeSpecifier::ULONG;
	content = val;
	return *this;
}

ODF& ODF::operator=(float val)
{
	type = TypeSpecifier::FLOAT;
	content = val;
	return *this;
}

ODF& ODF::operator=(double val)
{
	type = TypeSpecifier::DOUBLE;
	content = val;
	return *this;
}

ODF& ODF::operator=(const std::string& str)
{
	type = TypeSpecifier::CSTR;
	content = str;
	return *this;
}

ODF& ODF::operator=(const char* str)
{
	return *this = std::string(str);
}

ODF& ODF::operator=(const std::wstring& wstr)
{
	type = TypeSpecifier::WSTR;
	content = wstr;
	return *this;
}

ODF& ODF::operator=(const wchar_t* wstr)
{
	return *this = std::wstring(wstr);
}

ODF& ODF::operator=(const List& arr)
{
	type = arr.isMixed() ? TypeSpecifier::MXLIST : TypeSpecifier::FXLIST;
	content = arr;
	return *this;
}

ODF& ODF::operator=(const List::MixedArray& marr)
{
	type = TypeSpecifier::MXLIST;
	MixedArraySpecifier& marrSpec = std::get<MixedArraySpecifier>(std::get<ArraySpecifier>(*type.complexSpec));
	for (const ODF& odf : const_cast<List::MixedArray&>(marr).getArrayContainer())
		marrSpec.types.push_back(odf.type);
	content.emplace<List>() = marr;
	return *this;
}

ODF& ODF::operator=(const std::initializer_list<ODF>& initializer_list)
{
	return *this = List::MixedArray(initializer_list);
}

ODF& ODF::operator=(const List::FixedArray& farr)
{
	type = TypeSpecifier::FXLIST;
	std::get<FixedArraySpecifier>(std::get<ArraySpecifier>(*type.complexSpec)).fixType = farr.getType();
	content.emplace<List>() = List(farr);
	return *this;
}

ODF& ODF::operator=(const Object& obj)
{
	type = obj.isMixed() ? TypeSpecifier::MXOBJ : TypeSpecifier::FXOBJ;
	content = obj;
	return *this;
}

ODF& ODF::operator=(const Object::MixedObject& mobj)
{
	type = TypeSpecifier::MXOBJ;
	MixedObjectSpecifier& mobjSpec = std::get<MixedObjectSpecifier>(std::get<ObjectSpecifier>(*type.complexSpec));
	for (const Object::Pair& pair : const_cast<Object::MixedObject&>(mobj))
		mobjSpec.properties[pair.first] = pair.second.type;
	content.emplace<Object>() = mobj;
	return *this;
}

ODF& ODF::operator=(const std::initializer_list<std::variant<Object::Pair, ComplexExplicitor::OBJSTRUCT>>& initializer_list)
{
	return *this = Object::MixedObject(initializer_list);
}

ODF& ODF::operator=(const Object::FixedObject& fobj)
{
	type = TypeSpecifier::FXOBJ;
	FixedObjectSpecifier& mobjSpec = std::get<FixedObjectSpecifier>(std::get<ObjectSpecifier>(*type.complexSpec));
	mobjSpec.fixType = fobj.getType();
	content.emplace<Object>() = fobj;
	return *this;
}

ODF::Object& ODF::Object::operator=(const Object& other)
{
	if (&other == this)
		return *this;
	object = other.object;
	return *this;
}

ODF::Object& ODF::Object::operator=(const Object::MixedObject& mobj)
{
	object.emplace<Object::MixedObject>() = mobj;
	return *this;
}

ODF::Object& ODF::Object::operator=(const std::initializer_list<std::variant<Object::Pair, ComplexExplicitor::OBJSTRUCT>>& pairs)
{
	// copy the pairs into the current object, and skip all type marking objects (the ComplexExplicitor::OBJSTRUCT)
	return *this = MixedObject(pairs);
}

ODF::Object& ODF::Object::operator=(const Object::FixedObject& fobj)
{
	object.emplace<Object::FixedObject>() = fobj;
	return *this;
}

ODF::operator INT_8()
{
	return std::get<INT_8>(content);
}

ODF::operator UINT_8()
{
	return std::get<UINT_8>(content);
}

ODF::operator INT_16()
{
	return std::get<INT_16>(content);
}

ODF::operator UINT_16()
{
	return std::get<UINT_16>(content);
}

ODF::operator INT_32()
{
	return std::get<INT_32>(content);
}

ODF::operator UINT_32()
{
	return std::get<UINT_32>(content);
}

ODF::operator INT_64()
{
	return std::get<INT_64>(content);
}

ODF::operator UINT_64()
{
	return std::get<UINT_64>(content);
}

ODF::operator float()
{
	return std::get<float>(content);
}

ODF::operator double()
{
	return std::get<double>(content);
}

ODF::operator std::string()
{
	return std::get<std::string>(content);
}

ODF::operator std::wstring()
{
	return std::get<std::wstring>(content);
}

ODF::operator Object()
{
	return std::get<Object>(content);
}

ODF::operator List()
{
	return std::get<List>(content);
}

ODF::ODF() {}

ODF::ODF(const ODF& other)
{
	*this = other;
}

ODF::ODF(INT_8 val) : ODF()
{
	*this = val;
}

ODF::ODF(UINT_8 val) : ODF()
{
	*this = val;
}

ODF::ODF(INT_16 val) : ODF()
{
	*this = val;
}

ODF::ODF(UINT_16 val) : ODF()
{
	*this = val;
}

ODF::ODF(INT_32 val) : ODF()
{
	*this = val;
}

ODF::ODF(UINT_32 val) : ODF()
{
	*this = val;
}

ODF::ODF(INT_64 val) : ODF()
{
	*this = val;
}

ODF::ODF(UINT_64 val) : ODF()
{
	*this = val;
}

ODF::ODF(float val) : ODF()
{
	*this = val;
}

ODF::ODF(double val) : ODF()
{
	*this = val;
}

ODF::ODF(const std::string& str) : ODF()
{
	*this = str;
}

ODF::ODF(const char* cstr) : ODF()
{
	*this = std::string(cstr);
}

ODF::ODF(const std::wstring& wstr) : ODF()
{
	*this = wstr;
}

ODF::ODF(const wchar_t* wstr) : ODF()
{
	*this = std::wstring(wstr);
}

ODF::ODF(const Object& obj) : ODF()
{
	*this = obj;
}

ODF::ODF(const Object::MixedObject& mobj)
{
	*this = mobj;
}

ODF::ODF(const std::initializer_list<std::variant<Object::Pair, ComplexExplicitor::OBJSTRUCT>>& initializer_list)
{
	*this = initializer_list;
}

ODF::ODF(const Object::FixedObject& fobj)
{
	*this = fobj;
}

ODF::ODF(const List& arr) : ODF()
{
	*this = arr;
}

ODF::ODF(const List::MixedArray& marr) : ODF()
{
	*this = marr;
}

ODF::ODF(const List::FixedArray& farr) : ODF()
{
	*this = farr;
}

ODF::ODF(const std::initializer_list<ODF>& initializer_list) : ODF(List::MixedArray(initializer_list)) {}

ODF& ODF::immutable()
{
	type.makeImmutable();
	return *this;
}

bool ODF::isConvertableTo(const Type& other) const
{
	using TS = TypeSpecifier;
	switch (type.getswitch())
	{
	case TS::BYTE:
		switch (other.getswitch())
		{
		case TS::UBYTE:
			return std::get<INT_8>(content) == (UINT_8)std::get<INT_8>(content);
		case TS::BYTE:
		case TS::SHORT:
		case TS::USHORT:
		case TS::INT:
		case TS::UINT:
		case TS::LONG:
		case TS::ULONG:
			return true;
		default:
			return false;
		}
	case TS::UBYTE:
		switch (other.getswitch())
		{
		case TS::BYTE:
			return std::get<UINT_8>(content) == (UINT_8)std::get<UINT_8>(content);
		case TS::UBYTE:
		case TS::SHORT:
		case TS::USHORT:
		case TS::INT:
		case TS::UINT:
		case TS::LONG:
		case TS::ULONG:
			return true;
		default:
			return false;
		}
	case TS::SHORT:
		switch (other.getswitch())
		{
		case TS::UBYTE:
			return std::get<INT_16>(content) == (UINT_8)std::get<INT_16>(content);
		case TS::BYTE:
			return std::get<INT_16>(content) == (INT_8)std::get<INT_16>(content);
		case TS::USHORT:
			return std::get<INT_16>(content) == (UINT_16)std::get<INT_16>(content);
		case TS::SHORT:
		case TS::INT:
		case TS::UINT:
		case TS::LONG:
		case TS::ULONG:
			return true;
		default:
			return false;
		}
	case TS::USHORT:
		switch (other.getswitch())
		{
		case TS::UBYTE:
			return std::get<UINT_16>(content) == (UINT_8)std::get<UINT_16>(content);
		case TS::BYTE:
			return std::get<UINT_16>(content) == (INT_8)std::get<UINT_16>(content);
		case TS::SHORT:
			return std::get<UINT_16>(content) == (INT_16)std::get<UINT_16>(content);
		case TS::USHORT:
		case TS::INT:
		case TS::UINT:
		case TS::LONG:
		case TS::ULONG:
			return true;
		default:
			return false;
		}
	case TS::INT:
		switch (other.getswitch())
		{
		case TS::UBYTE:
			return std::get<INT_32>(content) == (UINT_8)std::get<INT_32>(content);
		case TS::BYTE:
			return std::get<INT_32>(content) == (INT_8)std::get<INT_32>(content);
		case TS::USHORT:
			return std::get<INT_32>(content) == (UINT_16)std::get<INT_32>(content);
		case TS::SHORT:
			return std::get<INT_32>(content) == (INT_16)std::get<INT_32>(content);
		case TS::UINT:
			return std::get<INT_32>(content) == (UINT_32)std::get<INT_32>(content);
		case TS::INT:
		case TS::LONG:
		case TS::ULONG:
			return true;
		default:
			return false;
		}
	case TS::UINT:
		switch (other.getswitch())
		{
		case TS::UBYTE:
			return std::get<UINT_32>(content) == (UINT_8)std::get<UINT_32>(content);
		case TS::BYTE:
			return std::get<UINT_32>(content) == (INT_8)std::get<UINT_32>(content);
		case TS::USHORT:
			return std::get<UINT_32>(content) == (UINT_16)std::get<UINT_32>(content);
		case TS::SHORT:
			return std::get<UINT_32>(content) == (INT_16)std::get<UINT_32>(content);
		case TS::INT:
			return std::get<UINT_32>(content) == (INT_32)std::get<UINT_32>(content);
		case TS::UINT:
		case TS::LONG:
		case TS::ULONG:
			return true;
		default:
			return false;
		}
	case TS::LONG:
		switch (other.getswitch())
		{
		case TS::UBYTE:
			return std::get<INT_64>(content) == (UINT_8)std::get<INT_64>(content);
		case TS::BYTE:
			return std::get<INT_64>(content) == (INT_8)std::get<INT_64>(content);
		case TS::USHORT:
			return std::get<INT_64>(content) == (UINT_16)std::get<INT_64>(content);
		case TS::SHORT:
			return std::get<INT_64>(content) == (INT_16)std::get<INT_64>(content);
		case TS::INT:
			return std::get<INT_64>(content) == (INT_32)std::get<INT_64>(content);
		case TS::UINT:
			return std::get<INT_64>(content) == (UINT_32)std::get<INT_64>(content);
		case TS::ULONG:
			return std::get<INT_64>(content) == (UINT_64)std::get<INT_64>(content);
		case TS::LONG:
			return true;
		default:
			return false;
		}
	case TS::ULONG:
		switch (other.getswitch())
		{
		case TS::UBYTE:
			return std::get<UINT_64>(content) == (UINT_8)std::get<UINT_64>(content);
		case TS::BYTE:
			return std::get<UINT_64>(content) == (INT_8)std::get<UINT_64>(content);
		case TS::USHORT:
			return std::get<UINT_64>(content) == (UINT_16)std::get<UINT_64>(content);
		case TS::SHORT:
			return std::get<UINT_64>(content) == (INT_16)std::get<UINT_64>(content);
		case TS::INT:
			return std::get<UINT_64>(content) == (INT_32)std::get<UINT_64>(content);
		case TS::UINT:
			return std::get<UINT_64>(content) == (UINT_32)std::get<UINT_64>(content);
		case TS::LONG:
			return std::get<UINT_64>(content) == (INT_64)std::get<UINT_64>(content);
		case TS::ULONG:
			return true;
		default:
			return false;
		}
	case TS::FLOAT:
		switch (other.getswitch())
		{
		case TS::FLOAT:
		case TS::DOUBLE:
			return true;
		default:
			return false;
		}
	case TS::DOUBLE:
		switch (other.getswitch())
		{
		case TS::DOUBLE:
			return true;
		default:
			return false;
		}
	case TS::CSTR:
		switch (other.getswitch())
		{
		case TS::CSTR:
			return true;
		default:
			return false;
		}
	case TS::WSTR:
		switch (other.getswitch())
		{
		case TS::WSTR:
			return true;
		default:
			return false;
		}
	}
}

void ODF::convertTo(const Type& newType)
{
	using TS = TypeSpecifier;
	try {
		switch (type.getswitch())
		{
		case TS::BYTE:
			switch (newType.getswitch())
			{
			case TS::BYTE:
				return;
			case TS::UBYTE:
				type = TS::UBYTE;
				content.emplace<UINT_8>(get<INT_8>()); // format: content.emplace<INNER>(get<OUTER>());
				return;
			case TS::SHORT:
				type = TS::SHORT;
				content.emplace<INT_16>(get<INT_8>());
				return;
			case TS::USHORT:
				type = TS::USHORT;
				content.emplace<UINT_16>(get<INT_8>());
				return;
			case TS::INT:
				type = TS::INT;
				content.emplace<INT_32>(get<INT_8>());
				return;
			case TS::UINT:
				type = TS::UINT;
				content.emplace<UINT_32>(get<INT_8>());
				return;
			case TS::LONG:
				type = TS::LONG;
				content.emplace<INT_64>(get<INT_8>());
				return;
			case TS::ULONG:
				type = TS::ULONG;
				content.emplace<UINT_64>(get<INT_8>());
				return;
			default:
				THROW InvalidConversion();
			}
		case TS::UBYTE:
			switch (newType.getswitch())
			{
			case TS::BYTE:
				type = TS::BYTE;
				content.emplace<INT_8>(get<UINT_8>());
				return;
			case TS::UBYTE:
				return;
			case TS::SHORT:
				type = TS::SHORT;
				content.emplace<INT_16>(get<UINT_8>());
				return;
			case TS::USHORT:
				type = TS::USHORT;
				content.emplace<UINT_16>(get<UINT_8>());
				return;
			case TS::INT:
				type = TS::INT;
				content.emplace<INT_32>(get<UINT_8>());
				return;
			case TS::UINT:
				type = TS::UINT;
				content.emplace<UINT_32>(get<UINT_8>());
				return;
			case TS::LONG:
				type = TS::LONG;
				content.emplace<INT_64>(get<UINT_8>());
				return;
			case TS::ULONG:
				type = TS::ULONG;
				content.emplace<UINT_64>(get<UINT_8>());
				return;
			default:
				THROW InvalidConversion();
			}
		case TS::SHORT:
			switch (newType.getswitch())
			{
			case TS::BYTE:
				type = TS::BYTE;
				content.emplace<INT_8>(get<INT_16>());
				return;
			case TS::UBYTE:
				type = TS::UBYTE;
				content.emplace<UINT_8>(get<INT_16>());
				return;
			case TS::SHORT:
				return;
			case TS::USHORT:
				type = TS::USHORT;
				content.emplace<UINT_16>(get<INT_16>());
				return;
			case TS::INT:
				type = TS::INT;
				content.emplace<INT_32>(get<INT_16>());
				return;
			case TS::UINT:
				type = TS::UINT;
				content.emplace<UINT_32>(get<INT_16>());
				return;
			case TS::LONG:
				type = TS::LONG;
				content.emplace<INT_64>(get<INT_16>());
				return;
			case TS::ULONG:
				type = TS::ULONG;
				content.emplace<UINT_64>(get<INT_16>());
				return;
			default:
				THROW InvalidConversion();
			}
		case TS::USHORT:
			switch (newType.getswitch())
			{
			case TS::BYTE:
				type = TS::BYTE;
				content.emplace<INT_8>(get<UINT_16>());
				return;
			case TS::UBYTE:
				type = TS::UBYTE;
				content.emplace<UINT_8>(get<UINT_16>());
				return;
			case TS::SHORT:
				type = TS::SHORT;
				content.emplace<INT_16>(get<UINT_16>());
				return;
			case TS::USHORT:
				return;
			case TS::INT:
				type = TS::INT;
				content.emplace<INT_32>(get<UINT_16>());
				return;
			case TS::UINT:
				type = TS::UINT;
				content.emplace<UINT_32>(get<UINT_16>());
				return;
			case TS::LONG:
				type = TS::LONG;
				content.emplace<INT_64>(get<UINT_16>());
				return;
			case TS::ULONG:
				type = TS::ULONG;
				content.emplace<UINT_64>(get<UINT_16>());
				return;
			default:
				THROW InvalidConversion();
			}
		case TS::INT:
			switch (newType.getswitch())
			{
			case TS::BYTE:
				type = TS::BYTE;
				content.emplace<INT_8>(get<INT_32>());
				return;
			case TS::UBYTE:
				type = TS::UBYTE;
				content.emplace<UINT_8>(get<INT_32>());
				return;
			case TS::SHORT:
				type = TS::SHORT;
				content.emplace<INT_16>(get<INT_32>());
				return;
			case TS::USHORT:
				type = TS::USHORT;
				content.emplace<UINT_16>(get<INT_32>());
				return;
			case TS::INT:
				return;
			case TS::UINT:
				type = TS::UINT;
				content.emplace<UINT_32>(get<INT_32>());
				return;
			case TS::LONG:
				type = TS::LONG;
				content.emplace<INT_64>(get<INT_32>());
				return;
			case TS::ULONG:
				type = TS::ULONG;
				content.emplace<UINT_64>(get<INT_32>());
				return;
			default:
				THROW InvalidConversion();
			}
		case TS::UINT:
			switch (newType.getswitch())
			{
			case TS::BYTE:
				type = TS::BYTE;
				content.emplace<INT_8>(get<UINT_32>());
				return;
			case TS::UBYTE:
				type = TS::UBYTE;
				content.emplace<UINT_8>(get<UINT_32>());
				return;
			case TS::SHORT:
				type = TS::SHORT;
				content.emplace<INT_16>(get<UINT_32>());
				return;
			case TS::USHORT:
				type = TS::USHORT;
				content.emplace<UINT_16>(get<UINT_32>());
				return;
			case TS::INT:
				type = TS::INT;
				content.emplace<INT_32>(get<UINT_32>());
				return;
			case TS::UINT:
				return;
			case TS::LONG:
				type = TS::LONG;
				content.emplace<INT_64>(get<UINT_32>());
				return;
			case TS::ULONG:
				type = TS::ULONG;
				content.emplace<UINT_64>(get<UINT_32>());
				return;
			default:
				THROW InvalidConversion();
			}
		case TS::LONG:
			switch (newType.getswitch())
			{
			case TS::BYTE:
				type = TS::BYTE;
				content.emplace<INT_8>(get<INT_64>());
				return;
			case TS::UBYTE:
				type = TS::UBYTE;
				content.emplace<UINT_8>(get<INT_64>());
				return;
			case TS::SHORT:
				type = TS::SHORT;
				content.emplace<INT_16>(get<INT_64>());
				return;
			case TS::USHORT:
				type = TS::USHORT;
				content.emplace<UINT_16>(get<INT_64>());
				return;
			case TS::INT:
				type = TS::INT;
				content.emplace<INT_32>(get<INT_64>());
				return;
			case TS::UINT:
				type = TS::UINT;
				content.emplace<UINT_32>(get<INT_64>());
				return;
			case TS::LONG:
				return;
			case TS::ULONG:
				type = TS::ULONG;
				content.emplace<UINT_64>(get<INT_64>());
				return;
			default:
				THROW InvalidConversion();
			}
		case TS::ULONG:
			switch (newType.getswitch())
			{
			case TS::BYTE:
				type = TS::BYTE;
				content.emplace<INT_8>(get<UINT_64>());
				return;
			case TS::UBYTE:
				type = TS::UBYTE;
				content.emplace<UINT_8>(get<UINT_64>());
				return;
			case TS::SHORT:
				type = TS::SHORT;
				content.emplace<INT_16>(get<UINT_64>());
				return;
			case TS::USHORT:
				type = TS::USHORT;
				content.emplace<UINT_16>(get<UINT_64>());
				return;
			case TS::INT:
				type = TS::INT;
				content.emplace<INT_32>(get<UINT_64>());
				return;
			case TS::UINT:
				type = TS::UINT;
				content.emplace<UINT_32>(get<UINT_64>());
				return;
			case TS::LONG:
				type = TS::LONG;
				content.emplace<INT_64>(get<UINT_64>());
				return;
			case TS::ULONG:
				return;
			default:
				THROW InvalidConversion();
			}
		case TS::FLOAT:
			switch (newType.getswitch())
			{
			case TS::FLOAT:
				return;
			case TS::DOUBLE:
				type = TS::DOUBLE;
				content.emplace<double>(get<float>());
				return;
			default:
				THROW InvalidConversion();
			}
		case TS::DOUBLE:
			switch (newType.getswitch())
			{
			case TS::DOUBLE:
				return;
			default:
				THROW InvalidConversion();
			}
			
		case TS::FXLIST: // TODO
		case TS::MXLIST: // TODO
		case TS::FXOBJ: // TODO
		case TS::MXOBJ: // TODO

		case TS::CSTR:
		case TS::WSTR:
			THROW InvalidConversion();
		}
	}
	catch (...)
	{
		THROW InvalidConversion();
	}
}

bool ODF::covertToIfConvertable(const Type& newType)
{
	if (isConvertableTo(newType))
	{
		convertTo(newType);
		return true;
	}
	else
		return false;
}

ODF ODF::getAs(const Type& newType) const
{
	ODF copy = *this;
	copy.convertTo(newType);
	return copy;
}

std::optional<ODF> ODF::getAsIfConvertable(const Type& newType) const
{
	if (isConvertableTo(newType))
	{
		ODF copy = *this;
		copy.convertTo(newType);
		return copy;
	}
	else
		return std::nullopt;
}

void ODF::makeList()
{
	List& list = content.emplace<List>();
	type = TypeSpecifier::MXLIST;
	list.clearAndMix();
}

void ODF::makeList(const Type& elementType)
{
	List& list = content.emplace<List>();
	type = TypeSpecifier::FXLIST;
	type.setFixType(elementType);
	list.clearAndFix(elementType);
}

void ODF::makeList(const TypeSpecifier& elementType)
{
	makeList(Type(elementType));
}

bool ODF::Type::operator!()
{
	return type == TypeSpecifier::NULLTYPE;
}

bool ODF::Type::operator==(const Type& other) const
{

	if (this->type != other.type) // check if primitive types match
		return false;
	if (this->isPrimitive()) // if those match they will be equal if either of them (both) are primitive
		return true;

	// compare complexSpecifiers
	if (this->isList())
	{
		if (this->isFixed())
		{
			ODF::FixedArraySpecifier* spec1 = this->fixedArraySpecifier();
			ODF::FixedArraySpecifier* spec2 = other.fixedArraySpecifier();
			return spec2->fixType == spec2->fixType && spec2->size.actualSize == spec2->size.actualSize;
		}
		else
		{
			ODF::MixedArraySpecifier* spec1 = this->mixedArraySpecifier();
			ODF::MixedArraySpecifier* spec2 = other.mixedArraySpecifier();
			if (spec1->types.size() != spec2->types.size())
				return false;
			for (size_t i = 0; i < spec1->types.size(); i++)
				if (spec1->types[i] != spec2->types[i])
					return false;
			return true;
		}
	}
	else
	{
		if (this->isFixed())
		{
			ODF::FixedObjectSpecifier* spec1 = this->fixedObjectSpecifier();
			ODF::FixedObjectSpecifier* spec2 = other.fixedObjectSpecifier();
			if (spec1->fixType != spec2->fixType)
				return false;
			if (spec1->keys.size() != spec2->keys.size())
				return false;
			for (size_t i = 0; i < spec1->keys.size(); i++)
				if (spec1->keys[i] != spec2->keys[i])
					return false;
			return true;
		}
		else
		{
			ODF::MixedObjectSpecifier* spec1 = this->mixedObjectSpecifier();
			ODF::MixedObjectSpecifier* spec2 = other.mixedObjectSpecifier();
			try {
				for (const std::pair<std::string, ODF::Type>& it : spec1->properties)
					if (spec2->properties.at(it.first) != it.second)
						return false;
			}
			catch (std::out_of_range&)
			{
				return false; // a key wasn't found
			}
			return true;
		}
	}
}

ODF::Type::operator unsigned char()
{
	return type;
}

ODF::Type& ODF::Type::operator=(const Type& other)
{
	if (&other == this)
		return *this;

	checkImmutable();
	immutable = other.immutable;
	type = other.type;
	if (complexSpec) delete complexSpec;
	complexSpec = nullptr;
	if (other.complexSpec)
	{
		complexSpec = new std::variant<ODF::ObjectSpecifier, ODF::ArraySpecifier>;
		*complexSpec = *other.complexSpec;
	}

	return *this;
}

ODF::TypeSpecifier ODF::Type::operator=(TypeSpecifier type)
{
	setType(type); // checkImmutable is called here
	return type;
}

ODF::Type::Type() : complexSpec(nullptr), immutable(false) {}

ODF::Type::Type(MemoryDataStream& mem, const ParseInfo& parseInfo, std::vector<VTypeInfo::TypeID>* dependecies)
{
	immutable = false;
	complexSpec = nullptr;
	ParseInfo localParseInfo = parseInfo;
	localParseInfo.dependecies = dependecies;
	Status error = this->loadFromMemory(mem, localParseInfo);
	if (error)
	{
		THROW error;
	}
}

ODF::Type::Type(const Type& other) : complexSpec(nullptr), immutable(false)
{
	*this = other;
}

ODF::Type::Type(Type&& type) noexcept
{
	this->immutable = type.immutable;
	this->complexSpec = type.complexSpec;
	type.complexSpec = nullptr;
	this->type = type.type;
}

ODF::Type::Type(TypeSpecifier type) : complexSpec(nullptr), immutable(false)
{
	*this = type;
}

ODF::Type::~Type()
{
}

ODF::Type ODF::Type::findBestType(TypeClass tc, unsigned char size, bool unsign)
{
	switch (tc)
	{
	case TypeClass::Int:
		switch (size)
		{
		case sizeof(INT_8):
			if (unsign) return Type(TypeSpecifier::UBYTE);
			else return Type(TypeSpecifier::BYTE);
		case sizeof(INT_16):
			if (unsign) return Type(TypeSpecifier::USHORT);
			else return Type(TypeSpecifier::SHORT);
		case sizeof(INT_32):
			if (unsign) return Type(TypeSpecifier::UINT);
			else return Type(TypeSpecifier::INT);
		case sizeof(INT_64):
			if (unsign) return Type(TypeSpecifier::ULONG);
			else return Type(TypeSpecifier::LONG);
		default:
			THROW InvalidCondition();
		}
	case TypeClass::Float:
		switch (size)
		{
		case sizeof(float):
			return Type(TypeSpecifier::FLOAT);
		case sizeof(double):
			return Type(TypeSpecifier::DOUBLE);
		default:
			THROW InvalidCondition();
		}
	case TypeClass::String:
		switch (size)
		{
		case 1:
			return Type(TypeSpecifier::CSTR);
		case 2:
			return Type(TypeSpecifier::WSTR);
		default:
			THROW InvalidCondition();
		}
	default:
		THROW InvalidCondition();
	}
}

#pragma endregion
#pragma region ObjectSpecifier
void ODF::MixedObjectSpecifier::saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const
{
	// CSTR key
	// TypeHeader value
	// empty key (just null terminator) = end of specifier
	for (const auto& it : iterationOrder)
	{
		// save key
		mem.writeStr(it);
		// save type
		properties.at(it).saveToMemory(mem, parseInfo);
	}
	// save zero to end object specifier
	mem.write<UINT_8>(0);
}

void ODF::MixedObjectSpecifier::loadFromMemory(MemoryDataStream& mem, const ParseInfo& parseInfo)
{
	properties.clear();
	std::string key;
	
	while (mem.peek())
	{
		key = mem.readStr();
		Type type;
		type.loadFromMemory(mem, parseInfo);
		properties[key] = type;
		iterationOrder.push_back(key);
	}
	mem.skip(); // skip null terminator that was peeked in the header of the while loop
}

void ODF::FixedObjectSpecifier::saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const
{
	// format:
	// TYPE type
	// CSTR key1
	// CSTR key2
	// ...
	// empty key = end

	// save type
	fixType.saveToMemory(mem, parseInfo);
	// save keys
	for (const std::string& key : keys)
		mem.writeStr(key);
	// save zero to end specifier
	mem.write<UINT_8>(0);
}

void ODF::FixedObjectSpecifier::loadFromMemory(MemoryDataStream& mem, const ParseInfo& parseInfo)
{
	// prepare object for reading
	keys.clear();
	
	// load header
	fixType.loadFromMemory(mem, parseInfo);

	// read keys until a null terminator
	while (mem.peek())
		keys.push_back(mem.readStr());
	mem.skip(); // skip null terminator
}

#pragma endregion
#pragma region SizeSpecifier
size_t ODF::SizeSpecifier::operator=(size_t size)
{
	actualSize = size;
	return size;
}
ODF::TypeSpecifier ODF::SizeSpecifier::sizeSpecifierSpecifier(TypeSpecifier original) const
{
	original = original & 0b00'111111ui8;
	if (actualSize < OverflowSize8bit)
		return 0b00'000000ui8 | (unsigned char)original;
	else if (actualSize < OverflowSize16bit)
		return 0b01'000000ui8 | (unsigned char)original;
	else if (actualSize < OverflowSize32bit)
		return 0b10'000000ui8 | (unsigned char)original;
	else
		return 0b11'000000ui8 | (unsigned char)original;
}

void ODF::SizeSpecifier::load(MemoryDataStream& mem, TypeSpecifier type)
{
	switch (type.operator unsigned char() >> 6) // looks cursed
	{
	case 0b00:
		actualSize = mem.read<UINT_8>();
		return;
	case 0b01:
		actualSize = mem.read<UINT_16>();
		return;
	case 0b10:
		actualSize = mem.read<UINT_32>();
		return;
	case 0b11:
		actualSize = mem.read<UINT_64>();
		return;
	}
}

void ODF::SizeSpecifier::saveToMemory(MemoryDataStream& mem, TypeSpecifier type) const
{
	switch (type & 0b11'000000ui8)
	{
	case 0b00'000000:
		mem.write((const char*)&actualSize, 1);
		return;
	case 0b01'000000:
		mem.write((const char*)&actualSize, 2);
		return;
	case 0b10'000000:
		mem.write((const char*)&actualSize, 4);
		return;
	case 0b11'000000:
		mem.write((const char*)&actualSize, 8);
		return;
	}
}

void ODF::SizeSpecifier::saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const
{
	// modify sizeSpecSpec in type
	mem.setPrevious(sizeSpecifierSpecifier(mem.peekPrevious()));
	saveToMemory(mem, sizeSpecifierSpecifier());
}

void ODF::SizeSpecifier::loadFromMemory(MemoryDataStream& mem, const ParseInfo& parseInfo)
{
	load(mem, mem.peekPrevious());
}

ODF::SizeSpecifier::SizeSpecifier() : actualSize(0) {}

#pragma endregion
#pragma endregion

ODF::Status ODF::saveToMemory(MemoryDataStream& mem) const
{
	// update the header if it includes specifiers that could have been altered after construction
	if (auto ptr = std::get_if<Object>(&content))
		ptr->updateKeys(std::get<ObjectSpecifier>(*type.complexSpec));
	else if (auto ptr = std::get_if<List>(&content))
		ptr->updateSpec(std::get<ArraySpecifier>(*type.complexSpec));

	ConstParseInfo parseInfo(pools, localPool, vtypeinfo);

	// save virtual types that were registered during loading, if the object was loaded in the past
	if (*vtypeinfo)
		vtypeinfo->saveToMemory(mem, parseInfo);

	// save header, reset flags
	parseInfo.flags = 0;
	if (Status error = type.saveToMemory(mem, parseInfo))
		return error;
	
	// save body
	return saveBody(mem);
}

ODF::Status ODF::saveToMemory(char* destination, size_t size) const
{
	MemoryDataStream mem(destination, size);
	return saveToMemory(mem);
}

ODF::Status ODF::loadFromMemory(MemoryDataStream& mem)
{
	// pools only need to be passed to the type loading, of this element and all of its childs, as that are all types of this translation unit.
	// pools are not needed in loadBody, as they are only linking types.

	ParseInfo parseInfo(pools, localPool, vtypeinfo);

repeat: // jump here to read again without recursion
	switch (Status error = type.loadFromMemory(mem, parseInfo))
	{
	case Status::Ok:
		break;
	case Status::Virtual:
		if (mem.sizeLeft()) // check if there is still something to read. If true, continue, else abort
			goto repeat;
		else
			return Status::Ok;
	default:
		return error;
	}

	return loadBody(mem);
}

void ODF::Type::setType(TypeSpecifier type)
{
	if (this->type != type)
		checkImmutable();
	if (complexSpec) delete complexSpec;

	this->type = type;

	if (isPrimitive())
		complexSpec = nullptr;
	else
	{
		complexSpec = new std::variant<ObjectSpecifier, ArraySpecifier>;
		if (isObject())
		{
			if (isFixed())
				complexSpec->emplace<ObjectSpecifier>().emplace<FixedObjectSpecifier>();
			else if (isMixed())
				complexSpec->emplace<ObjectSpecifier>().emplace<MixedObjectSpecifier>();
			else
			{
				THROW InvalidCondition();
			}
		}
		else if (isList())
		{
			if (isFixed())
				complexSpec->emplace<ArraySpecifier>().emplace<FixedArraySpecifier>();
			else if (isMixed())
				complexSpec->emplace<ArraySpecifier>().emplace<MixedArraySpecifier>();
			else
			{
				THROW InvalidCondition();
			}
		}
		else
		{
			THROW InvalidCondition();
		}
	}
}

inline bool ODF::Type::isPrimitive() const
{
	return !(type.isObject() || type.isList());
}

inline bool ODF::Type::needsObjectSpecifier() const
{
	return type.isObject();
}

inline bool ODF::Type::needsSizeSpecifier() const
{
	// fixed list is the only type specified type currently
	return type.withoutSSS() == TypeSpecifier::FXLIST;
}

inline bool ODF::Type::isFixed() const
{
	return type.isFixed();
}

bool ODF::TypeSpecifier::isMixed() const
{
	return type & FLAG_MIXED;
}

bool ODF::TypeSpecifier::isFixed() const
{
	// if type is list or object and FLAG_MIXED is not set
	return (type & FLAG_OBJ || type & FLAG_LIST) && !(type & FLAG_MIXED);
}

bool ODF::TypeSpecifier::isObject() const
{
	return smallType() == FLAG_OBJ;
}

bool ODF::TypeSpecifier::isList() const
{
	return type & FLAG_LIST;
}

ODF::TypeSpecifier::Type ODF::TypeSpecifier::smallType() const
{
	return type & 0b0001'1111;
}

ODF::TypeSpecifier::Type ODF::TypeSpecifier::withoutSSS() const
{
	return type & 0b00'111111;
}

bool ODF::TypeSpecifier::isVirtual() const
{
	return isVirtual(type);
}

bool ODF::TypeSpecifier::isVirtual(unsigned char type)
{
	type &= 0b0001'1111;
	return (type == DEFTYPE) || (type == IMPORT) || (type == EXPORT) || !isBuiltIn(type);
}

bool ODF::TypeSpecifier::isBuiltIn(unsigned char type)
{
	const uint8_t high_nibble = type >> 4;

	// 1. Exclude the "Special" high-nibble rows 0x0n and 0x2n, which iclude all types
	if (high_nibble == 0x0 || high_nibble == 0x2) return true;

	// 2. Exclude shadows of the size specified types
	// If the 2 MSBs are used, the type is defined by the lower 6 bits (b & 0x3F).
	const uint8_t withoutSSS = type & 0x3F;
	if (withoutSSS == FXLIST || withoutSSS == DEFTYPE || withoutSSS == IMPORT || withoutSSS == EXPORT || withoutSSS == USETYPE || withoutSSS == IMPORTAS || withoutSSS == EXPORTAS)
		return true;

	return false;
}

void ODF::TypeSpecifier::saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const
{
	mem.write(type);
}

void ODF::TypeSpecifier::loadFromMemory(MemoryDataStream& mem, const ParseInfo& parseInfo)
{
	*this = mem.read<Type>();
}

inline bool ODF::Type::isMixed() const
{
	return type.isMixed();
}

inline bool ODF::Type::isObject() const
{
	return type.isObject();
}

inline bool ODF::Type::isList() const
{
	return type.isList();
}

ODF::Status ODF::loadFromMemory(const char* data, size_t size)
{
	MemoryDataStream mem(data, size);
	return loadFromMemory(mem);
}

ODF::Status ODF::saveToFile(std::string file) const
{
	MemoryDataStream mem(0ui64); // DBG; TODO. (remove constructor argument to enable dynamic allocation for release build)
	saveToMemory(mem);
	
	std::ofstream out(file, std::ios::binary);
	if (!out)
		return Status::FileOpenError;
	
	out.write(mem.data(), mem.size());
	if (!out)
	{
		out.close();
		return Status::FileWriteError;
	}

	out.close();
	if (!out)
		return Status::FileCloseError;

	return Status::Ok;
}

ODF::Status ODF::loadFromFile(std::string file)
{
	std::ifstream in(file, std::ios::binary);
	if (!in)
		return Status::FileOpenError;

	size_t size = std::filesystem::file_size(file);
	char* data = new char[size];
	
	in.read(data, size);
	if (!in)
	{
		delete[] data;
		in.close();
		return Status::FileReadError;
	}

	in.close();
	if (!in)
		return Status::FileCloseError;

	loadFromMemory(data, size);
	delete[] data;
	return Status::Ok;
}

ODF::Status ODF::saveToStream(std::ostream& out)
{
	MemoryDataStream mem;
	saveToMemory(mem);

	out.write(mem.data(), mem.size());
	return Status::Ok;
}

ODF::Status ODF::loadFromStream(std::istream& in)
{
	// the vector should not be copied because NRVO
	std::vector<char> data = read_all_bytes(in);
	MemoryDataStream mem(data.data(), data.size());
	loadFromMemory(mem);
	return Status::Ok;
}

ODF::Status ODF::saveBody(MemoryDataStream& mem) const
{
	// primtive types
	try {
		switch (type.getVariantType())
		{
		case VT_INT8:
			mem.write(std::get<INT_8>(content));
			return Status::Ok;
		case VT_UINT8:
			mem.write(std::get<UINT_8>(content));
			return Status::Ok;
		case VT_INT16:
			mem.write(std::get<INT_16>(content));
			return Status::Ok;
		case VT_UINT16:
			mem.write(std::get<UINT_16>(content));
			return Status::Ok;
		case VT_INT32:
			mem.write(std::get<INT_32>(content));
			return Status::Ok;
		case VT_UINT32:
			mem.write(std::get<UINT_32>(content));
			return Status::Ok;
		case VT_INT64:
			mem.write(std::get<INT_64>(content));
			return Status::Ok;
		case VT_UINT64:
			mem.write(std::get<UINT_64>(content));
			return Status::Ok;
		case VT_FLOAT:
			mem.write(std::get<float>(content));
			return Status::Ok;
		case VT_DOUBLE:
			mem.write(std::get<double>(content));
			return Status::Ok;
		case VT_CSTR:
			mem.writeStr(std::get<std::string>(content));
			return Status::Ok;
		case VT_WSTR:
			mem.writeWstr(std::get<std::wstring>(content));
			return Status::Ok;
		case VT_OBJ:
			return saveObject(mem);
		case VT_LIST:
			return saveArray(mem);
		}
	}
	catch (std::bad_variant_access&)
	{
		return Status::TypeMismatch;
	}
}

ODF::Status ODF::saveObject(MemoryDataStream& mem) const
{
	std::get<Object>(content).saveToMemory(mem, std::get<ObjectSpecifier>(*type.complexSpec));
	return Status::Ok;
}

ODF::Status ODF::saveArray(MemoryDataStream& mem) const
{
	std::get<List>(content).saveToMemory(mem);		
	return Status::Ok;
}

ODF::Status ODF::loadBody(MemoryDataStream& mem)
{
	switch (type.getVariantType())
	{
	case VT_INT8:
		content = mem.read<INT_8>();
		return Status::Ok;
	case VT_UINT8:
		content = mem.read<UINT_8>();
		return Status::Ok;
	case VT_INT16:
		content = mem.read<INT_16>();
		return Status::Ok;
	case VT_UINT16:
		content = mem.read<UINT_16>();
		return Status::Ok;
	case VT_INT32:
		content = mem.read<INT_32>();
		return Status::Ok;
	case VT_UINT32:
		content = mem.read<UINT_32>();
		return Status::Ok;
	case VT_INT64:
		content = mem.read<INT_64>();
		return Status::Ok;
	case VT_UINT64:
		content = mem.read<UINT_64>();
		return Status::Ok;
	case VT_FLOAT:
		content = mem.read<float>();
		return Status::Ok;
	case VT_DOUBLE:
		content = mem.read<double>();
		return Status::Ok;
	case VT_CSTR:
		content = mem.readStr();
		return Status::Ok;
	case VT_WSTR:
		content = mem.readWstr();
		return Status::Ok;
	case VT_OBJ:
		return loadObject(mem);
	case VT_LIST:
		return loadArray(mem);
	default:
		THROW InvalidDefault();
	}
	return Status::Ok;
}

ODF::Status ODF::loadObject(MemoryDataStream& mem)
{
	Object& obj = content.emplace<Object>();
	obj.loadFromMemory(mem, std::get<ObjectSpecifier>(*type.complexSpec));
	return Status::Ok;
}

ODF::Status ODF::loadArray(MemoryDataStream& mem)
{
	List& arr = content.emplace<List>();
	arr.loadFromMemory(mem, std::get<ArraySpecifier>(*type.complexSpec));
	return Status::Ok;
}

ODF::Status::operator Value()
{
	return content;
}

bool ODF::Status::operator!()
{
	return !bool(*this);
}

bool ODF::Status::operator!=(Status other)
{
	return content != other.content;
}

bool ODF::Status::operator!=(Value other)
{
	return content != other;
}

bool ODF::Status::operator==(Status other)
{
	return content == other.content;
}

bool ODF::Status::operator==(Value other)
{
	return content == other;
}

ODF::Status ODF::Status::operator=(Status other)
{
	if (&other == this)
		return *this;
	content = other.content;
	return *this;
}

ODF::Status::Status(Value val)
{
	content = val;
}

void ODF::push_back(const ODF& odf)
{
	try {
		std::get<List>(content).push_back(odf);
	}
	catch (std::bad_variant_access&)
	{
		content.emplace<List>().push_back(odf);
	}
}

void ODF::push_front(const ODF& odf)
{
	try {
		std::get<List>(content).push_front(odf);
	}
	catch (std::bad_variant_access&)
	{
		content.emplace<List>().push_front(odf);
	}
}

void ODF::erase(size_t index)
{
	std::get<List>(content).erase(index);
}

void ODF::erase(size_t index, size_t numberOfElements)
{
	std::get<List>(content).erase(index, numberOfElements);
}

void ODF::erase_range(size_t from, size_t to)
{
	std::get<List>(content).erase_range(from, to);
}

void ODF::insert(size_t where, const ODF& odf)
{
	try {
		std::get<List>(content).insert(where, odf);
	}
	catch (std::bad_variant_access&)
	{
		content.emplace<List>().insert(where, odf);
	}
}

std::optional<size_t> ODF::find(const ODF& odf)
{
	return std::get<List>(content).find(odf);
}

bool ODF::push_back_nothrow(const ODF& odf) noexcept
{
	try {
		push_back(odf);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool ODF::push_front_nothrow(const ODF& odf) noexcept
{
	try {
		push_front(odf);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool ODF::erase_nothrow(size_t index) noexcept
{
	try {
		erase(index);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool ODF::erase_nothrow(size_t index, size_t numberOfElements) noexcept
{
	try {
		erase(index, numberOfElements);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool ODF::erase_range_nothrow(size_t from, size_t to) noexcept
{
	try {
		erase_range(from, to);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool ODF::insert_nothrow(size_t where, const ODF& odf) noexcept
{
	try {
		insert(where, odf);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

std::optional<size_t> ODF::find_nothrow(const ODF& odf) noexcept
{
	try {
		return find(odf);
	}
	catch (...)
	{
		return (size_t)-1;
	}
}

void ODF::makeObject()
{
	Object& obj = content.emplace<Object>();
	type = TypeSpecifier::MXOBJ;
	obj.clearAndMix();
}

void ODF::makeObject(const Type& elementType)
{
	List& list = content.emplace<List>();
	type = TypeSpecifier::FXLIST;
	type.setFixType(elementType);
	list.clearAndFix(elementType);

	Object& obj = content.emplace<Object>();
	type = TypeSpecifier::FXOBJ;
	type.setFixType(elementType);
	obj.clearAndFix(elementType);
}

void ODF::makeObject(const TypeSpecifier& elementType)
{
	makeObject(Type(elementType));
}

void ODF::insert(const std::string& key, const ODF& odf)
{
	try {
		std::get<Object>(content).insert(key, odf);
	}
	catch (std::bad_variant_access&)
	{
		content.emplace<Object>().insert(key, odf);
	}
}

void ODF::insert(const Pair& value)
{
	try {
		std::get<Object>(content).insert(value);
	}
	catch (std::bad_variant_access&)
	{
		content.emplace<Object>().insert(value);
	}
}

void ODF::insert(const std::unordered_map<std::string, ODF>& map)
{
	try {
		std::get<Object>(content).insert(map);
	}
	catch (std::bad_variant_access&)
	{
		content.emplace<Object>().insert(map);
	}
}

void ODF::insert(const std::map<std::string, ODF>& umap)
{
	try {
		std::get<Object>(content).insert(umap);
	}
	catch (std::bad_variant_access&)
	{
		content.emplace<Object>().insert(umap);
	}
}

void ODF::insert(const std::vector<Pair>& pairs)
{
	try {
		std::get<Object>(content).insert(pairs);
	}
	catch (std::bad_variant_access&)
	{
		content.emplace<Object>().insert(pairs);
	}
}

void ODF::insert(const std::initializer_list<Pair>& pairs)
{
	try {
		std::get<Object>(content).insert(pairs);
	}
	catch (std::bad_variant_access&)
	{
		content.emplace<Object>().insert(pairs);
	}
}

void ODF::erase(const std::string& key)
{
	std::get<Object>(content).erase(key);
}

bool ODF::Object::contains(const std::string& key) const
{
	return std::visit([&](const AbstractObject& aobj) -> bool {
		return aobj.contains(key);
		}, object);
}

size_t ODF::Object::count(const std::string& key) const
{
	return std::visit([&](const AbstractObject& aobj) -> size_t {
		return aobj.count(key);
		}, object);
}

size_t ODF::Object::size() const
{
	return std::visit([&](const AbstractObject& aobj) -> size_t {
		return aobj.size();
		}, object);
}

bool ODF::contains(const std::string& key) const
{
	return std::get<Object>(content).contains(key);
}

size_t ODF::count(const std::string& key) const
{
	return std::get<Object>(content).count(key);
}

size_t ODF::size() const
{
	if (auto ptr = std::get_if<List>(&content))
		return ptr->size();
	else if (auto ptr = std::get_if<Object>(&content))
		return ptr->size();
	return type.getSize();
}

void ODF::clear()
{
	if (auto ptr = std::get_if<List>(&content))
		ptr->clear();
	else if (auto ptr = std::get_if<Object>(&content))
		ptr->clear();
}

void ODF::List::FixedArray::push_back(const ODF& odf)
{
	// check type, then push_back, otherwise register fail
	Type realType = odf.getComplexType();
	if (fixType.type == TypeSpecifier::NULLTYPE) // if fixtype is NULL it will be set correctly
		fixType = realType; // trigger next if statement. COmpiler will propably optimaize this out
	if (realType == fixType)
	{
		list.push_back(odf);
		list.back().immutable(); // have this here, because the else block handles immutabillity seperately by recursively calling push_back
	}
	else
	{
		// try converting the element to the desired type
		if (odf.isConvertableTo(fixType))
			push_back(odf.getAs(fixType));
		else
		{
			// thrown an exception if it is not convertable
			THROW TypeMismatch();
		}
	}
}

void ODF::List::FixedArray::push_front(const ODF& odf)
{
	// check type, then push_back, otherwise register fail
	Type realType = odf.getComplexType();
	if (fixType.type == TypeSpecifier::NULLTYPE) // if fixtype is NULL it will be set correctly
		fixType = realType; // trigger next if statement. COmpiler will propably optimaize this out
	if (realType == fixType)
	{
		push_front(odf);
		list.front().immutable(); // have this here, because the else block handles immutabillity seperately by recursively calling push_front
	}
	else
	{
		// try converting the element to the desired type
		if (odf.isConvertableTo(fixType))
			push_front(odf.getAs(fixType));
		else
		{
			// thrown an exception if it is not convertable
			THROW TypeMismatch();
		}
	}
}

void ODF::List::FixedArray::erase(size_t index)
{
	list.erase(list.begin() + index);
}

void ODF::List::FixedArray::erase(size_t index, size_t numberOfElements)
{
	if (!numberOfElements)
		return;
	list.erase(list.begin() + index, list.begin() + index + numberOfElements - 1);
}

void ODF::List::FixedArray::erase_range(size_t from, size_t to)
{
	list.erase(list.begin() + from, list.begin() + to);
}

void ODF::List::FixedArray::insert(size_t where, const ODF& odf)
{
	// check type, then push_back, otherwise register fail
	Type realType = odf.getComplexType();
	if (fixType.type == TypeSpecifier::NULLTYPE) // if fixtype is NULL it will be set correctly
		fixType = realType; // trigger next if statement. COmpiler will propably optimaize this out
	if (realType == fixType)
	{
		list.insert(list.begin() + where, odf);
		list[where + 1].immutable(); // where is the index of the elemet before the new elememt // have this here, because the else block handles immutabillity seperately by recursively calling insert
	}
	else
	{
		// try converting the element to the desired type
		if (odf.isConvertableTo(fixType))
			list.insert(list.begin() + where, odf.getAs(fixType));
		else
		{
			// thrown an exception if it is not convertable
			THROW TypeMismatch();
		}
	}
}

std::optional<size_t> ODF::List::FixedArray::find(const ODF& odf) const
{
	auto it = std::find(list.begin(), list.end(), odf);
	return it == list.end() ? std::nullopt : std::make_optional(std::distance(list.begin(), it));
}

size_t ODF::List::FixedArray::size() const
{
	return list.size();
}

void ODF::List::FixedArray::clear()
{
	list.clear();
}

void ODF::List::FixedArray::updateSize(ODF::Type& type) const
{
	*type.sizeSpecifier() = list.size();
	type.setSSS(type.sizeSpecifier()->sizeSpecifierSpecifier());
}

void ODF::List::FixedArray::setType(const Type& type)
{
	if (list.empty())
	{
		fixType = type;
		return;
	}
	if (!isConvertableTo(type))
	{
		THROW InvalidConversion();
	}
	convertTo(type);
}

bool ODF::List::FixedArray::isConvertableTo(const Type& newFixType) const
{
	// check every element if it is convertable to newFixType.
	// simply checking for fixtype isn't sufficient as isConvertableTo also depends on the actual value in stored in the ODF object
	for (const ODF& odf : list)
		if (!odf.isConvertableTo(newFixType))
			return false;
	return true;
}

bool ODF::List::FixedArray::convertTo(const Type& type)
{
	fixType = type;
	for (ODF& odf : list)
		odf.convertTo(fixType);
	return true;
}

bool ODF::List::FixedArray::convertTo(std::function<Type(const std::vector<ODF>&)> type, std::function<void(ODF& odf)> converter)
{
	fixType = type(list);
	for (ODF& odf : list)
		converter(odf);
	return true;
}

void ODF::List::FixedArray::resize(size_t newSize)
{
	list.resize(newSize);
	for (ODF& odf : list)
		odf.type = fixType;
}

ODF& ODF::List::FixedArray::operator[](size_t index)
{
	return list[index];
}

const ODF& ODF::List::FixedArray::operator[](size_t index) const
{
	return list[index];
}

ODF& ODF::List::FixedArray::at(size_t index)
{
	return list.at(index);
}

const ODF& ODF::List::FixedArray::at(size_t index) const
{
	return list[index];
}

ODF::List::ConstIterator ODF::List::FixedArray::begin() const noexcept
{
	return list.begin();
}

ODF::List::Iterator ODF::List::FixedArray::begin() noexcept
{
	return list.begin();
}

ODF::List::ConstIterator ODF::List::FixedArray::end() const noexcept
{
	return list.end();
}

ODF::List::Iterator ODF::List::FixedArray::end() noexcept
{
	return list.end();
}

std::vector<ODF>& ODF::List::FixedArray::getArrayContainer()
{
	return list;
}

inline const ODF::Type& ODF::List::FixedArray::getType() const
{
	return fixType;
}

void ODF::List::FixedArray::updateSpec(ArraySpecifier& spec) const
{
	FixedArraySpecifier& fspec = std::get<FixedArraySpecifier>(spec);
	fspec.size = list.size();
}

void ODF::List::FixedArray::saveToMemory(MemoryDataStream& mem) const
{
	for (const IteratorType& it : list)
		it.saveBody(mem);
}

void ODF::List::FixedArray::loadFromMemory(MemoryDataStream& mem, const ArraySpecifier& spec)
{
	// clear existing data
	list.clear();

	// resize list and load each item
	const FixedArraySpecifier& fspec = std::get<FixedArraySpecifier>(spec);
	list.resize(fspec.size.actualSize);
	fixType = fspec.fixType;
	for (ODF& odf : list)
	{
		odf.type = fspec.fixType;
		odf.loadBody(mem);
		odf.immutable();
	}
}

ODF::List::FixedArray::FixedArray()
{
}

void ODF::List::MixedArray::updateSpec(ArraySpecifier& spec) const
{
	// clear old type data
	MixedArraySpecifier& mspec = std::get<MixedArraySpecifier>(spec);
	mspec.types.clear();

	// re-add type data
	mspec.types.reserve(list.size());
	for (const ODF& odf : list)
		mspec.types.push_back(odf.type);
}

void ODF::List::MixedArray::saveToMemory(MemoryDataStream& mem) const
{
	for (const IteratorType& it : list)
		it.saveBody(mem);
}

void ODF::List::MixedArray::loadFromMemory(MemoryDataStream& mem, const ArraySpecifier& spec)
{
	// clear old data
	list.clear();

	// load new data with specifiers
	const MixedArraySpecifier& mspec = std::get<MixedArraySpecifier>(spec);
	list.resize(mspec.types.size());
	for (size_t i = 0; i < list.size(); i++)
	{
		list[i].type = mspec.types[i];
		list[i].loadBody(mem);
	}
}

ODF::List::MixedArray::MixedArray()
{
}

ODF::List::MixedArray::MixedArray(const std::initializer_list<ODF>& initializer_list)
{
	for (const ODF& odf : initializer_list)
		list.push_back(odf);
}

ODF::List::FixedArray::InvalidTypeChange::InvalidTypeChange(std::string message) : runtime_error(message) {}

ODF::Type ODF::Object::canBeFixed() const
{
	if (!size())
		return Type(TypeSpecifier::UNDEFINED);
	// first check if all elements have the same type class
	const MixedObject& mixed = std::get<MixedObject>(object);
	const ODF& first = mixed.begin()->second;
	TypeClass tc = first.getTypeClass();
	bool unsign = first.type.isUnsigned();

	// check all elemets (first elements is check twice but i'll leave it like that. And if we're already iterating, find the largest type size
	unsigned char largestSize = 0;
	for (const IteratorType& it : mixed)
	{
		if (tc != it.second.getTypeClass())
			return Type(TypeSpecifier::NULLTYPE);
		if (unsign != it.second.type.isUnsigned())
			return Type(TypeSpecifier::NULLTYPE);
		largestSize = std::max(largestSize, it.second.type.getSize());
	}

	// now return the type
	return Type::findBestType(tc, largestSize, unsign);
}

bool ODF::Object::tryFixing()
{
	Type newFixType = canBeFixed();
	if (!newFixType)
		return false;

	MixedObject old = std::get<MixedObject>(object);
	FixedObject& fobj = object.emplace<FixedObject>();

	for (const IteratorType& it : old)
		fobj.insert(it.first, it.second.getAs(newFixType));
	return true;
}

bool ODF::Object::isMixed() const
{
	return object.index();
}

ODF::Type ODF::Object::getFixedDatatype() const
{
	return std::get<FixedObject>(object).getType();
}

ODF::Object::FixedObject& ODF::Object::fixed()
{
	return std::get<FixedObject>(object);
}

ODF::Object::MixedObject& ODF::Object::mixed()
{
	return std::get<MixedObject>(object);
}

ODF::Object::Object()
{
}

ODF::Object::Object(const Object& other) : Object()
{
	*this = other;
}

ODF::Object::Object(const Object::MixedObject& mobj) : Object()
{
	*this = mobj;
}

ODF::Object::Object(const std::initializer_list<std::variant<Pair, ComplexExplicitor::OBJSTRUCT>>& pairs) : Object(MixedObject(pairs))
{
	*this = pairs;
}

ODF::Object::Object(const Object::FixedObject& fobj)
{
	*this = fobj;
}

void ODF::List::clear()
{
	std::visit([](AbstractArray& arr) {
		arr.clear();
		}, list);
}

void ODF::List::resize(size_t newSize)
{
	std::visit([&](AbstractArray& aarr) {
		aarr.resize(newSize);
		}, list);
}

void ODF::Object::updateKeys(ObjectSpecifier& spec) const
{
	std::visit([&](const AbstractObject& aobj) {
		aobj.updateKeys(spec);
		}, object);
}

void ODF::Object::loadFromMemory(MemoryDataStream& mem, const ObjectSpecifier& spec)
{
	if (spec.index())
		object.emplace<FixedObject>().loadFromMemory(mem, spec);
	else
		object.emplace<MixedObject>().loadFromMemory(mem, spec);
}

void ODF::Object::saveToMemory(MemoryDataStream& mem, const ObjectSpecifier& spec) const
{
	std::visit([&mem, &spec](const AbstractObject& aobj) {
		aobj.saveToMemory(mem, spec);
		}, object);
}

void ODF::Object::clear()
{
	std::visit([&](AbstractObject& aobj) {
		aobj.clear();
		}, object);
}

ODF::Object::ConstIterator ODF::Object::begin() const noexcept
{
	return std::visit([&](const AbstractObject& aobj) -> ConstIterator {
		return aobj.begin();
		}, object);
}

ODF::Object::Iterator ODF::Object::begin() noexcept
{
	return std::visit([&](AbstractObject& aobj) -> Iterator {
		return aobj.begin();
		}, object);
}

ODF::Object::ConstIterator ODF::Object::end() const noexcept
{
	return std::visit([&](const AbstractObject& aobj) -> ConstIterator {
		return aobj.end();
		}, object);
}

ODF::Object::Iterator ODF::Object::end() noexcept
{
	return std::visit([&](AbstractObject& aobj) -> Iterator {
		return aobj.end();
		}, object);
}

std::unordered_map<std::string, ODF>& ODF::Object::getObjectContainer()
{
	if (auto ptr = std::get_if<FixedObject>(&object))
		return ptr->map;
	else
		return std::get<MixedObject>(object).map;
}

void ODF::Object::clearAndFix()
{
	FixedObject& fobj = object.emplace<FixedObject>();
	fobj.clear();
	fobj.setType(Type(TypeSpecifier::NULLTYPE));
}

void ODF::Object::clearAndFix(const ODF::Type& elementType)
{
	FixedObject& fobj = object.emplace<FixedObject>();
	fobj.clear();
	fobj.setType(elementType);
}

void ODF::Object::clearAndMix()
{
	object.emplace<MixedObject>().clear();
}

void ODF::Object::makeMixed()
{
	FixedObject old = std::get<FixedObject>(object);
	MixedObject& mixed = object.emplace<MixedObject>();

	for (const IteratorType& it : old)
		mixed.insert(it);
}

void ODF::List::clearAndFix()
{
	FixedArray& farr = list.emplace<FixedArray>();
	farr.clear();
	farr.setType(Type(TypeSpecifier::NULLTYPE));
}

void ODF::List::clearAndFix(const Type& elementType)
{
	list.emplace<FixedArray>().clear();
	std::get<FixedArray>(list).setType(elementType);
}

void ODF::List::clearAndMix()
{
	list.emplace<MixedArray>().clear();
}

void ODF::List::makeMixed()
{
	FixedArray old = std::get<FixedArray>(list);
	MixedArray& mixed = list.emplace<MixedArray>();

	for (const ODF& odf : old)
		mixed.push_back(odf);
}

ODF::Type ODF::List::canBeFixed() const
{
	if (!size())
		return Type(TypeSpecifier::UNDEFINED);
	// first check if all elements have the same type class
	const MixedArray& mixed = std::get<MixedArray>(list);
	const ODF& first = *mixed.begin();
	TypeClass tc = first.getTypeClass();
	bool unsign = first.type.isUnsigned();

	// check all elemets (first elements is check twice but i'll leave it like that. And if we're already iterating, find the largest type size
	unsigned char largestSize = 0;
	for (const ODF& it : mixed)
	{
		if (tc != it.getTypeClass())
			return Type(TypeSpecifier::NULLTYPE);
		if (unsign != it.type.isUnsigned())
			return Type(TypeSpecifier::NULLTYPE);
		largestSize = std::max(largestSize, it.type.getSize());
	}

	// now return the type
	return Type::findBestType(tc, largestSize, unsign);
}

bool ODF::List::tryFixing()
{
	Type newFixType = canBeFixed();
	if (!newFixType)
		return false;

	MixedArray old = std::get<MixedArray>(list);
	FixedArray& farr = list.emplace<FixedArray>();

	for (ODF& odf : old)
		farr.push_back(odf.getAs(newFixType));
	return true;
}

bool ODF::List::isMixed() const
{
	return list.index();
}

ODF::Type ODF::List::getFixedDatatype() const
{
	return std::get<FixedArray>(list).getType();
}

void ODF::List::push_back(const ODF& odf)
{
	std::visit([&odf](ODF::AbstractArray& base) {
		base.push_back(odf);
		}, list);
}

void ODF::List::push_front(const ODF& odf)
{
	std::visit([&odf](ODF::AbstractArray& base) {
		base.push_front(odf);
		}, list);
}

void ODF::List::erase(size_t index)
{
	std::visit([&index](ODF::AbstractArray& base) {
		base.erase(index);
		}, list);
}

void ODF::List::erase(size_t index, size_t numberOfElements)
{
	std::visit([&](ODF::AbstractArray& base) {
		base.erase(index, numberOfElements);
		}, list);
}

void ODF::List::erase_range(size_t from, size_t to)
{
	std::visit([&](ODF::AbstractArray& base) {
		base.erase_range(from, to);
		}, list);
}

void ODF::List::insert(size_t where, const ODF& odf)
{
	std::visit([&](ODF::AbstractArray& base) {
		base.insert(where, odf);
		}, list);
}

std::optional<size_t> ODF::List::find(const ODF& odf) const
{
	return std::visit([&](const ODF::AbstractArray& base) -> std::optional<size_t> {
		return base.find(odf);
		}, list);
}

size_t ODF::List::size() const
{
	return std::visit([](const ODF::AbstractArray& base) {
		return base.size();
		}, list);
}

ODF& ODF::List::operator[](size_t index)
{
	return std::visit([&](AbstractArray& base) -> ODF& { return base[index]; }, list);
}

const ODF& ODF::List::operator[](size_t index) const
{
	return std::visit([&](const AbstractArray& base) -> const ODF& { return base[index]; }, list);
}

ODF& ODF::List::at(size_t index)
{
	return std::visit([&](AbstractArray& aarr) -> ODF& { return aarr.at(index); }, list);
}

const ODF& ODF::List::at(size_t index) const
{
	return std::visit([&](const AbstractArray& aarr) -> const ODF& { return aarr.at(index); }, list);
}

ODF::List::ConstIterator ODF::List::begin() const noexcept
{
	return std::visit([](const AbstractArray& aarr) -> ODF::List::ConstIterator {
		return aarr.begin();
		}, list);
}

ODF::List::Iterator ODF::List::begin() noexcept
{
	return std::visit([](AbstractArray& aarr) -> ODF::List::Iterator {
		return aarr.begin();
		}, list);
}

ODF::List::ConstIterator ODF::List::end() const noexcept
{
	return std::visit([](const AbstractArray& aarr) -> ODF::List::ConstIterator {
		return aarr.end();
		}, list);
}

ODF::List::Iterator ODF::List::end() noexcept
{
	return std::visit([](AbstractArray& aarr) -> ODF::List::Iterator {
		return aarr.end();
		}, list);
}

std::vector<ODF>& ODF::List::getArrayContainer()
{
	if (auto ptr = std::get_if<MixedArray>(&list))
		return ptr->getArrayContainer();
	else if (auto ptr = std::get_if<FixedArray>(&list))
		return ptr->getArrayContainer();
	else
	{
		THROW InvalidCondition();
	}
}

ODF::List::FixedArray& ODF::List::fixed() const
{
	return const_cast<FixedArray&>(std::get<FixedArray>(list));
}

ODF::List::MixedArray& ODF::List::mixed() const
{
	return const_cast<MixedArray&>(std::get<MixedArray>(list));
}

void ODF::List::resetAndSetSize(size_t newSize, const std::vector<Type>& newTypes)
{
	MixedArray& marr = list.emplace<MixedArray>();
	marr.clear();
	marr.resize(newSize);
	if (marr.size() != newTypes.size())
	{
		THROW SpecifierMismatch();
	}
	for (size_t i = 0; i < marr.size(); i++)
		marr[i].type = newTypes[i];
}

void ODF::List::resetAndSetSize(size_t newSize, const Type& newType)
{
	FixedArray& farr = list.emplace<FixedArray>();
	farr.clear();
	farr.setType(newType); // set Type befire resizing, as in resize every elemet gets the fixType
	farr.resize(newSize);
}

void ODF::List::updateSpec(ArraySpecifier& spec) const
{
	std::visit([&](const AbstractArray& aarr) {
		aarr.updateSpec(spec);
		}, list);
}

void ODF::List::saveToMemory(MemoryDataStream& mem) const
{
	std::visit([&](const AbstractArray& aarr) {
		aarr.saveToMemory(mem);
		}, list);
}

void ODF::List::loadFromMemory(MemoryDataStream& mem, const ArraySpecifier& spec)
{
	if (spec.index())
		list.emplace<FixedArray>().loadFromMemory(mem, spec);
	else
		list.emplace<MixedArray>().loadFromMemory(mem, spec);
}

ODF::List& ODF::List::operator=(const List& other)
{
	if (&other == this)
		return *this;

	list = other.list;
	return *this;
}

ODF::List& ODF::List::operator=(const MixedArray& other)
{
	list = other;
	return *this;
}

ODF::List& ODF::List::operator=(const std::initializer_list<ODF>& initializer_list)
{
	*this = MixedArray(initializer_list);
	return *this;
}

ODF::List& ODF::List::operator=(const FixedArray& other)
{
	list = other;
	return *this;
}

ODF::List::List()
{
}

ODF::List::List(const List& arr) : List()
{
	*this = arr;
}

ODF::List::List(const MixedArray& marr) : List()
{
	*this = marr;
}

ODF::List::List(const std::initializer_list<ODF>& initializer_list) : List()
{
	*this = initializer_list;
}

ODF::List::List(const FixedArray& farr) : List()
{
	*this = farr;
}

ODF::VariantTypeConversionError::VariantTypeConversionError(std::string message) : runtime_error(message)
{
}

void ODF::FixedArraySpecifier::saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const
{
	size.saveToMemory(mem, parseInfo);
	fixType.saveToMemory(mem, parseInfo);
}

void ODF::FixedArraySpecifier::loadFromMemory(MemoryDataStream& mem, const ParseInfo& parseInfo)
{
	size.loadFromMemory(mem, parseInfo);
	fixType.loadFromMemory(mem, parseInfo);
}

ODF::SixSeven::SixSeven(const std::string& message) : runtime_error(message) {}

ODF::TypeMismatch::TypeMismatch(const std::string& message) : runtime_error(message) {}

void ODF::MixedArraySpecifier::saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const
{
	for (const Type& type : types)
		type.saveToMemory(mem, parseInfo);
	mem.write(0ui8); // NULLTYPE
}

void ODF::MixedArraySpecifier::loadFromMemory(MemoryDataStream& mem, const ParseInfo& parseInfo)
{
	// load types until a NULL type is found
	while (mem.peek()) // next read type is not NULL
	{
		Type type;
		type.loadFromMemory(mem, parseInfo);
		types.push_back(type);
	}
	mem.skip(); // skip null terminator
}

ODF::InvalidCondition::InvalidCondition(const std::string& message) : runtime_error(message) {}

ODF::SpecifierMismatch::SpecifierMismatch(const std::string& message) : runtime_error(message) {}

ODF::InvalidType::InvalidType(const std::string& message) : std::runtime_error(message) {}

ODF::InvalidConversion::InvalidConversion(const std::string& message) : std::runtime_error(message) {}

ODF& ODF::at(size_t index)
{
	return std::get<List>(content).at(index);
}

const ODF& ODF::at(size_t index) const
{
	return std::get<List>(content).at(index);
}

ODF& ODF::at(const std::string& key)
{
	return std::get<Object>(content).at(key);
}

const ODF& ODF::at(const std::string& key) const
{
	return std::get<Object>(content).at(key);
}

void ODF::Object::MixedObject::updateKeys(ObjectSpecifier& spec) const
{
	// delete old properties
	MixedObjectSpecifier& mspec = std::get<MixedObjectSpecifier>(spec);
	mspec.properties.clear();

	// insert new properties
	for (const Pair& it : map)
		mspec.properties[it.first] = it.second.type;
}

void ODF::Object::MixedObject::saveToMemory(MemoryDataStream& mem, const ObjectSpecifier& spec) const
{
	// save bodies seperately
	const MixedObjectSpecifier& mspec = std::get<MixedObjectSpecifier>(spec);
	for (const auto& it : mspec.iterationOrder)
		map.at(it).saveBody(mem);
}

void ODF::Object::MixedObject::loadFromMemory(MemoryDataStream& mem, const ObjectSpecifier& spec)
{
	// clear previous content
	map.clear();

	// add all keys and their type, while simultaniously loading the elements in the correct order
	const MixedObjectSpecifier& mspec = std::get<MixedObjectSpecifier>(spec);
	for (const auto& it : mspec.iterationOrder)
	{
		ODF& newElement = map[it];
		newElement.type = mspec.properties.at(it);
		newElement.loadBody(mem);
	}
}

ODF::Object::MixedObject::MixedObject()
{
}

ODF::Object::MixedObject::MixedObject(const std::initializer_list<Pair>& elements)
{
	for (const Pair& it : elements)
		insert(it);
}

ODF::Object::MixedObject::MixedObject(const std::initializer_list<std::variant<Pair, ComplexExplicitor::OBJSTRUCT>>& pairs)
{
	for (const std::variant<Pair, ComplexExplicitor::OBJSTRUCT>& it : pairs)
		if (auto ptr = std::get_if<Pair>(&it))
			insert(*ptr);
}

void ODF::Object::FixedObject::insert(const std::string& key, const ODF& odf)
{
	Type realType = odf.getComplexType();
	if (fixType.type == TypeSpecifier::NULLTYPE) // if fix type is null it will be set
		fixType = realType;
	if (realType == fixType) // only insert matching types
		map.insert(std::make_pair(key, odf));
	else
	{
		// insertion failed because of a type mismatch. Try to convert the elemet to the desired type. Throw if still not convertable
		if (odf.isConvertableTo(fixType))
			map.insert(std::make_pair(key, odf.getAs(fixType)));
		else
		{
			THROW TypeMismatch();
		}
	}
	map[key].type.makeImmutable();
}

void ODF::Object::FixedObject::insert(const Pair& pair)
{
	Type realType = pair.second.getComplexType();
	if (fixType.type == TypeSpecifier::NULLTYPE) // if fix type is null it will be set
		fixType = realType;
	if (realType == fixType) // only insert matching types
		map.insert(pair);
	else
	{
		// insertion failed because of a type mismatch. Try to convert the elemet to the desired type. Throw if still not convertable
		if (pair.second.isConvertableTo(fixType))
			map.insert(std::make_pair(pair.first, pair.second.getAs(fixType)));
		else
		{
			THROW TypeMismatch();
		}
	}
	map[pair.first].type.makeImmutable();
}

void ODF::Object::FixedObject::insert(const std::map<std::string, ODF>& map)
{
	for (const IteratorType& it : map)
		insert(it);
}

void ODF::Object::FixedObject::insert(const std::unordered_map<std::string, ODF>& umap)
{
	for (const IteratorType& it : umap)
		insert(it);
}

void ODF::Object::FixedObject::insert(const std::vector<Pair>& pairs)
{
	for (const Pair& it : pairs)
		insert(it);
}

void ODF::Object::FixedObject::insert(const std::initializer_list<Pair>& pairs)
{
	for (const Pair& it : pairs)
		insert(it);
}

void ODF::Object::FixedObject::erase(const std::string& key)
{
	map.erase(key);
}

bool ODF::Object::FixedObject::contains(const std::string& key) const
{
	return map.contains(key);
}

size_t ODF::Object::FixedObject::count(const std::string& key) const
{
	return map.count(key);
}

size_t ODF::Object::FixedObject::size() const
{
	return map.size();
}

void ODF::Object::FixedObject::clear()
{
	map.clear();
}

ODF::Object::AbstractObject::ConstIterator ODF::Object::FixedObject::begin() const noexcept
{
	return map.begin();
}

ODF::Object::Iterator ODF::Object::FixedObject::begin() noexcept
{
	return map.begin();
}

ODF::Object::AbstractObject::ConstIterator ODF::Object::FixedObject::end() const noexcept
{
	return map.end();
}

ODF::Object::Iterator ODF::Object::FixedObject::end() noexcept
{
	return map.end();
}

std::unordered_map<std::string, ODF>& ODF::Object::FixedObject::getObjectContainer()
{
	return map;
}

ODF& ODF::Object::FixedObject::operator[](const std::string& key)
{
	if (auto element = map.find(key); element != map.end())
		return element->second;
	else
		return map[key].immutable();
}

ODF& ODF::Object::FixedObject::at(const std::string& key)
{
	return map.at(key); // no need for immutable(), as every element is made immutable on insertion
}

const ODF& ODF::Object::FixedObject::at(const std::string& key) const
{
	return map.at(key); // no need for immutable(), as every element is made immutable on insertion
}

void ODF::Object::FixedObject::setType(const Type& type)
{
	if (map.empty())
	{
		fixType = type;
		return;
	}
	if (!isConvertableTo(type))
	{
		THROW InvalidConversion();
	}
	convertTo(type);
}

bool ODF::Object::FixedObject::isConvertableTo(const Type& newFixType) const
{
	// check every element if it is convertable to newFixType.
	// simply checking for fixtype isn't sufficient as isConvertableTo also depends on the actual value in stored in the ODF object
	for (const IteratorType& it : map)
		if (!it.second.isConvertableTo(newFixType))
			return false;
	return true;
}

bool ODF::Object::FixedObject::convertTo(const Type& type)
{
	fixType = type;
	for (IteratorType& pair : map)
		pair.second.convertTo(fixType);
	return true;
}

bool ODF::Object::FixedObject::convertTo(std::function<Type(const std::unordered_map<std::string, ODF>&)> type, std::function<void(ODF& odf)> converter)
{
	fixType = type(map);
	for (IteratorType& pair : map)
		converter(pair.second);
	return true;
}

const ODF::Type& ODF::Object::FixedObject::getType() const
{
	return fixType;
}

void ODF::Object::FixedObject::updateKeys(ObjectSpecifier& spec) const
{
	FixedObjectSpecifier& fspec = std::get<FixedObjectSpecifier>(spec);
	fspec.keys.clear();
	fspec.keys.reserve(map.size());

	for (const Pair& it : map)
		fspec.keys.push_back(it.first);
}

void ODF::Object::FixedObject::saveToMemory(MemoryDataStream& mem, const ObjectSpecifier& spec) const
{
	// save bodies seperately
	const FixedObjectSpecifier& fspec = std::get<FixedObjectSpecifier>(spec);
	for (const auto& it : fspec.keys)
		map.at(it).saveBody(mem);
}

void ODF::Object::FixedObject::loadFromMemory(MemoryDataStream& mem, const ObjectSpecifier& spec)
{
	// clear all previous elemets
	map.clear();

	// add all keys, and load all bodies in the same order
	const FixedObjectSpecifier& fspec = std::get<FixedObjectSpecifier>(spec);
	fixType = fspec.fixType;
	for (const std::string& it : fspec.keys)
	{
		ODF& newElement = map[it];
		newElement.type = fixType;
		newElement.loadBody(mem);
		newElement.immutable();
	}
}

ODF::Object::FixedObject::FixedObject()
{
}

ODF::InvalidTypeMutation::InvalidTypeMutation(const std::string& message) : runtime_error(message) {}

void ODF::Object::insert(const std::string& key, const ODF& odf)
{
	std::visit([&](AbstractObject& aobj) {
		aobj.insert(key, odf);
		}, object);
}

void ODF::Object::insert(const Pair& value)
{
	std::visit([&](AbstractObject& aobj) {
		aobj.insert(value);
		}, object);
}

void ODF::Object::insert(const std::map<std::string, ODF>& map)
{
	std::visit([&](AbstractObject& aobj) {
		aobj.insert(map);
		}, object);
}

void ODF::Object::insert(const std::unordered_map<std::string, ODF>& umap)
{
	std::visit([&](AbstractObject& aobj) {
		aobj.insert(umap);
		}, object);
}

void ODF::Object::insert(const std::vector<Pair>& pairs)
{
	std::visit([&](AbstractObject& aobj) {
		aobj.insert(pairs);
		}, object);
}

void ODF::Object::insert(const std::initializer_list<Pair>& pairs)
{
	std::visit([&](AbstractObject& aobj) {
		aobj.insert(pairs);
		}, object);
}

void ODF::Object::erase(const std::string& key)
{
	std::visit([&](AbstractObject& aobj) {
		aobj.erase(key);
		}, object);
}

ODF& ODF::Object::operator[](const std::string& key)
{
	return std::visit([&](AbstractObject& aobj) -> ODF& {
		return aobj[key];
		}, object);
}

ODF& ODF::Object::at(const std::string& key)
{
	return std::visit([&](AbstractObject& aobj) -> ODF& {
		return aobj.at(key);
		}, object);
}

const ODF& ODF::Object::at(const std::string& key) const
{
	return std::visit([&](const AbstractObject& aobj) -> const ODF& {
		return aobj.at(key);
		}, object);
}

void ODF::Object::MixedObject::insert(const std::string& key, const ODF& odf)
{
	map.insert(std::make_pair(key, odf));
}

void ODF::Object::MixedObject::insert(const Pair& value)
{
	map.insert(value);
}

void ODF::Object::MixedObject::insert(const std::map<std::string, ODF>& map)
{
	for (const Pair& it : map)
		this->map.insert(it);
}

void ODF::Object::MixedObject::insert(const std::unordered_map<std::string, ODF>& umap)
{
	for (const Pair& it : umap)
		map.insert(it);
}

void ODF::Object::MixedObject::insert(const std::vector<Pair>& pairs)
{
	for (const Pair& it : pairs)
		map.insert(it);
}

void ODF::Object::MixedObject::insert(const std::initializer_list<Pair>& pairs)
{
	for (const Pair& it : pairs)
		map.insert(it);
}

void ODF::Object::MixedObject::erase(const std::string& key)
{
	map.erase(key);
}

bool ODF::Object::MixedObject::contains(const std::string& key) const
{
	return map.contains(key);
}

size_t ODF::Object::MixedObject::count(const std::string& key) const
{
	return map.count(key);
}

size_t ODF::Object::MixedObject::size() const
{
	return map.size();
}

void ODF::Object::MixedObject::clear()
{
	map.clear();
}

ODF& ODF::Object::MixedObject::operator[](const std::string& key)
{
	return map[key];
}

ODF& ODF::Object::MixedObject::at(const std::string& key)
{
	return map.at(key);
}

const ODF& ODF::Object::MixedObject::at(const std::string& key) const
{
	return map.at(key);
}

ODF::Object::ConstIterator ODF::Object::MixedObject::begin() const noexcept
{
	return map.begin();
}

ODF::Object::Iterator ODF::Object::MixedObject::begin() noexcept
{
	return map.begin();
}

ODF::Object::ConstIterator ODF::Object::MixedObject::end() const noexcept
{
	return map.end();
}

ODF::Object::Iterator ODF::Object::MixedObject::end() noexcept
{
	return map.end();
}

std::unordered_map<std::string, ODF>& ODF::Object::MixedObject::getObjectContainer()
{
	return map;
}

void ODF::List::MixedArray::push_back(const ODF& odf)
{
	list.push_back(odf);
}

void ODF::List::MixedArray::push_front(const ODF& odf)
{
	list.insert(list.begin(), odf);
}

void ODF::List::MixedArray::erase(size_t index)
{
	list.erase(list.begin() + index);
}

void ODF::List::MixedArray::erase(size_t index, size_t numberOfElements)
{
	if (!numberOfElements)
		return;
	list.erase(list.begin() + index, list.begin() + index + numberOfElements - 1);
}

void ODF::List::MixedArray::erase_range(size_t from, size_t to)
{
	list.erase(list.begin() + from, list.begin() + to);
}

void ODF::List::MixedArray::insert(size_t where, const ODF& odf)
{
	list.insert(list.begin() + where, odf);
}

std::optional<size_t> ODF::List::MixedArray::find(const ODF& odf) const
{
	auto it = std::find(list.begin(), list.end(), odf);
	return it == list.end() ? std::nullopt : std::make_optional(std::distance(list.begin(), it));
}

size_t ODF::List::MixedArray::size() const
{
	return list.size();
}

void ODF::List::MixedArray::clear()
{
	list.clear();
}

void ODF::List::MixedArray::resize(size_t newSize)
{
	list.resize(newSize);
}

ODF& ODF::List::MixedArray::operator[](size_t index)
{
	return list[index];
}

const ODF& ODF::List::MixedArray::operator[](size_t index) const
{
	return list[index];
}

ODF& ODF::List::MixedArray::at(size_t index)
{
	return list.at(index);
}

const ODF& ODF::List::MixedArray::at(size_t index) const
{
	return list.at(index);
}

ODF::List::ConstIterator ODF::List::MixedArray::begin() const noexcept
{
	return list.begin();
}

ODF::List::Iterator ODF::List::MixedArray::begin() noexcept
{
	return list.begin();
}

ODF::List::ConstIterator ODF::List::MixedArray::end() const noexcept
{
	return list.end();
}

ODF::List::Iterator ODF::List::MixedArray::end() noexcept
{
	return list.end();
}

std::vector<ODF>& ODF::List::MixedArray::getArrayContainer()
{
	return list;
}

void ODF::PoolCollection::addImportPool(Pool pool)
{
	importPools.push_back(pool);
}

void ODF::PoolCollection::addExportPool(Pool pool)
{
	exportPools.push_back(pool);
}

void ODF::usePools()
{
	pools = std::make_shared<PoolCollection>();
}

void ODF::resetPools()
{
	pools.reset();
}

void ODF::addPool(Pool pool)
{
	pools->addPool(pool);
}

void ODF::addImportPool(Pool pool)
{
	pools->addImportPool(pool);
}

void ODF::addExportPool(Pool pool)
{
	pools->addExportPool(pool);
}

ODF::Pool ODF::operator+=(Pool pool)
{
	addPool(pool);
	return pool;
}

const ODF::PoolCollection& ODF::operator=(const PoolCollection& pc)
{
	return *(pools = std::make_shared<PoolCollection>(pc)); // the old one is automatically deallocated by std::shared_ptr::operator=
}

ODF::ODF(const PoolCollection& pc)
{
	*this = pc;
}

void ODF::PoolCollection::addPool(Pool pool)
{
	importPools.push_back(pool);
	exportPools.push_back(pool);
}

std::optional<ODF::Type> ODF::PoolCollection::importType(size_t id) const
{
	// search all pools for this type and return it.
	for (const Pool& it : importPools)
	{
		try {
			return it->at(TypeID(id, 0)); // the sss is ignored in the hashmap. the hash of a a TypeID with a specific runtime id will be the same, as if the runtimeID would have been hashed alone.
		}
		catch (std::out_of_range&) {} // continue if element not found
	}
	// element not found
	return std::nullopt;
}

std::optional<ODF::Type> ODF::PoolCollection::importType(TypeID id) const
{
	// search all pools for this type and return it.
	for (const Pool& it : importPools)
	{
		try {
			return it->at(id);
		}
		catch (std::out_of_range&) {} // continue if element not found
	}
	// element not found
	return std::nullopt;
}

void ODF::PoolCollection::exportType(TypeID id, const Type& type)
{
	// add the type to all exportPools
	for (Pool& it : exportPools)
		it->insert(std::make_pair(id, type));
}

void ODF::PoolCollection::exportType(const std::pair<TypeID, const Type&>& pair)
{
	for (Pool& it : exportPools)
		it->insert(pair);
}

std::optional<std::pair<ODF::Type, ODF::VTypeInfo::TypeID>> ODF::PoolCollection::parse(MemoryDataStream& mem, const std::shared_ptr<PoolType>& localPool, const std::shared_ptr<VTypeInfo>& vtypeinfo)
{
	using TypeID = VTypeInfo::TypeID;
	using TS = TypeSpecifier;

	// parse the virtual type and return it. Only USETYPE returns a type, all other virtaul types must be callable without a body, so they do not return a type.

	// Every information about all pools will be written into th vtypeinfo, so the exact same virtual type structure can be reproduced on saving.
	
	// read TypeSpecifier
	unsigned char vtype = mem.read();

	// declare variables for switch statement
	Type type;
	VTypeInfo::TypeIDWithDependencies key;

	// little note on memory mangement:
	// The PoolCollection inherits from std::enable_shared_from_this to get access to itself in a shared_ptr.
	// For the construction of the parseInfo struct a shared_ptr to the PoolCollection is needed, which is the current object.
	// This shared_ptr to itself is provided by std::enable_shared_from_this. It is important that the current object is already contained in a shared_ptr, else a std::bad_weak_ptr is thrown
	ParseInfo parseInfo(shared_from_this(), localPool, vtypeinfo);

	switch (vtype & 0b0011'1111)
	{
	case TS::DEFTYPE:
		// errors from the constructor are handled as exceptions, which aren't caught, as this function also handles error by throwing
		
		switch (getVTypeSizeSpec(vtype))
		{
			// this syntax in the case blocks may seem a little weird.
			// first the type is loaded by its constructor.
			// This type is than saved in the localPool and added as a key into defiendTypes which points to the typeID, which is used as key by the localPool.
		case 0:
			key = getFullTypeID(mem.read<UINT_8>(), true);
			vtypeinfo->addDefinedType((*localPool)[key] = Type(mem, parseInfo, &key.dependencies), key);
			return std::nullopt;
		case 1:
			key = getFullTypeID(mem.read<UINT_8>(), false);
			vtypeinfo->addDefinedType((*localPool)[key] = Type(mem, parseInfo, &key.dependencies), key);
			return std::nullopt;
		case 2:
			key = getFullTypeID(mem.read<UINT_16>(), false);
			vtypeinfo->addDefinedType((*localPool)[key] = Type(mem, parseInfo, &key.dependencies), key);
			return std::nullopt;
		case 3:
			key = getFullTypeID(mem.read<UINT_32>(), false);
			vtypeinfo->addDefinedType((*localPool)[key] = Type(mem, parseInfo, &key.dependencies), key);
			return std::nullopt;
		}
		break;

	case TS::USETYPE:
		try
		{
			TypeID tid;
			switch (getVTypeSizeSpec(vtype))
			{
			case 0:
				tid = getFullTypeID(mem.read<UINT_8>(), true);
				break;
			case 1:
				tid = getFullTypeID(mem.read<UINT_8>(), false);
				break;
			case 2:
				tid = getFullTypeID(mem.read<UINT_16>(), false);
				break;
			case 3:
				tid = getFullTypeID(mem.read<UINT_32>(), false);
				break;
			}
			return std::make_pair(localPool->at(tid), tid);
		}
		catch (std::out_of_range&)
		{
			THROW Status::InvalidLocalTypeID;
		}
		break;
	case TS::EXPORT:
		try
		{
			TypeID typeID;
			switch (getVTypeSizeSpec(vtype))
			{
			case 0:
				typeID = getFullTypeID(mem.read<UINT_8>(), true);
				typeID.sss = 0;
				break;
			case 1:
				typeID = getFullTypeID(mem.read<UINT_8>(), false);
				typeID.sss = 1;
				break;
			case 2:
				typeID = getFullTypeID(mem.read<UINT_16>(), false);
				typeID.sss = 2;
				break;
			case 3:
				typeID = getFullTypeID(mem.read<UINT_32>(), false);
				typeID.sss = 3;
				break;
			}
			exportType(typeID, localPool->at(typeID));
			vtypeinfo->addExport(typeID.runtimeID, typeID.sss);
			return std::nullopt;
		}
		catch (std::out_of_range&)
		{
			THROW Status::InvalidLocalTypeID;
		}
		break;
	case TS::EXPORTAS:
		try
		{
			TypeID localID, globalID;
			switch (getVTypeSizeSpec(vtype))
			{
			case 0:
				localID = getFullTypeID(mem.read<UINT_8>(), true);
				globalID = getFullTypeID(mem.read<UINT_8>(), true);
				localID.sss = 0; // as these two variable aren't used anywhere else than after this switch statement, it is not neccesarry to fill the sss of globalID
				break;
			case 1:
				localID = getFullTypeID(mem.read<UINT_8>(), false);
				globalID = getFullTypeID(mem.read<UINT_8>(), false);
				localID.sss = 1;
				break;
			case 2:
				localID = getFullTypeID(mem.read<UINT_16>(), false);
				globalID = getFullTypeID(mem.read<UINT_16>(), false);
				localID.sss = 2;
				break;
			case 3:
				localID = getFullTypeID(mem.read<UINT_32>(), false);
				globalID = getFullTypeID(mem.read<UINT_32>(), false);
				localID.sss = 3;
				break;
			default:
				THROW InvalidCondition(); // to prevent compiler warning for uninitialized variable globalID in the next line.
			}
			exportType(globalID, localPool->at(localID));
			vtypeinfo->addExport(localID.runtimeID, globalID.runtimeID, localID.sss);
			return std::nullopt;
		}
		catch (std::out_of_range&)
		{
			THROW Status::InvalidLocalTypeID; // The typeID that should be exported wasn't found
		}
		break;
	case TS::IMPORT:
		try
		{
			TypeID typeID;
			switch (getVTypeSizeSpec(vtype))
			{
			case 0:
				typeID = getFullTypeID(mem.read<UINT_8>(), true);
				typeID.sss = 0;
				break;
			case 1:
				typeID = getFullTypeID(mem.read<UINT_8>(), false);
				typeID.sss = 1;
				break;
			case 2:
				typeID = getFullTypeID(mem.read<UINT_16>(), false);
				typeID.sss = 2;
				break;
			case 3:
				typeID = getFullTypeID(mem.read<UINT_32>(), false);
				typeID.sss = 3;
				break;
			default:
				THROW InvalidCondition(); // to prevent compiler warning for uninitialized variable typeID in the next line.
			}
			(*localPool)[typeID] = importType(typeID.runtimeID).value();
			vtypeinfo->addImport(typeID.runtimeID, typeID.sss);
			return std::nullopt;
		}
		catch (std::bad_optional_access&)
		{
			THROW Status::InvalidLocalTypeID;
		}
		break;
	case TS::IMPORTAS:
		try
		{
			TypeID globalID, localID;
			switch (getVTypeSizeSpec(vtype))
			{
			case 0:
				globalID = getFullTypeID(mem.read<UINT_8>(), true);
				localID = getFullTypeID(mem.read<UINT_8>(), true);
				globalID.sss = 0; // as these two variable aren't used anywhere else than after this switch statement, it is not neccesarry to fill the sss of localID
				break;
			case 1:
				globalID = getFullTypeID(mem.read<UINT_8>(), false);
				localID = getFullTypeID(mem.read<UINT_8>(), false);
				globalID.sss = 1;
				break;
			case 2:
				globalID = getFullTypeID(mem.read<UINT_16>(), false);
				localID = getFullTypeID(mem.read<UINT_16>(), false);
				globalID.sss = 2;
				break;
			case 3:
				globalID = getFullTypeID(mem.read<UINT_32>(), false);
				localID = getFullTypeID(mem.read<UINT_32>(), false);
				globalID.sss = 3;
				break;
			default:
				THROW InvalidCondition(); // to prevent compiler warning for uninitialized variable globalID in the next line.
			}
			(*localPool)[localID] = importType(globalID).value();
			vtypeinfo->addImport(globalID.runtimeID, localID.runtimeID, globalID.sss);
			return std::nullopt;
		}
		catch (std::bad_optional_access&)
		{
			THROW Status::InvalidLocalTypeID;
		}
		break;
	default:
		TypeID tid = getFullTypeID(vtype, true);
		return std::make_pair(parseInfo.importType(tid).value(), tid);
	}
	THROW InvalidCondition("InvalidCondition: End of Function 'ODF::PoolCollection::parse(MemoryDataStream&, PoolType&)' reached.");
}

unsigned char ODF::PoolCollection::getVTypeSizeSpec(unsigned char vtype)
{
	return vtype >> 6;
}

ODF::VTypeInfo::TypeID ODF::PoolCollection::getFullTypeID(UINT_8 id, bool replacesOldID)
{
	// return id if replacesOldID is false. else return id | 0xFF00'0000'0000'0000
	return TypeID(replacesOldID ? (id | 0xFF00'0000'0000'0000) : id, !replacesOldID);
}

ODF::VTypeInfo::TypeID ODF::PoolCollection::getFullTypeID(UINT_16 id, bool replacesOldID)
{
	// return id if replacesOldID is false. else return id | 0xFF00'0000'0000'0000
	return TypeID(replacesOldID ? (id | 0xFF00'0000'0000'0000) : id, replacesOldID ? 0 : 2);
}

ODF::VTypeInfo::TypeID ODF::PoolCollection::getFullTypeID(UINT_32 id, bool replacesOldID)
{
	// return id if replacesOldID is false. else return id | 0xFF00'0000'0000'0000
	return TypeID(replacesOldID ? (id | 0xFF00'0000'0000'0000) : id, replacesOldID ? 0 : 3);
}

std::pair<UINT_32, bool> ODF::PoolCollection::makeFullTypeID(size_t fullTypeID)
{
	size_t dbg = fullTypeID & 0xFF00'0000'0000'0000;
	return std::make_pair(static_cast<UINT_32>(fullTypeID) & 0xFFFF'FFFF, (fullTypeID & 0xFF00'0000'0000'0000) == 0xFF00'0000'0000'0000);
}

ODF::Pool ODF::PoolCollection::operator=(Pool pool)
{
	importPools.clear();
	exportPools.clear();
	addPool(pool);
	return pool;
}

ODF::Pool ODF::PoolCollection::operator+=(Pool pool)
{
	addPool(pool);
	return pool;
}

ODF::PoolCollection::PoolCollection(Pool pool)
{
	addPool(pool);
}

ODF::PoolCollection::PoolCollection(Pool importPool, Pool exportPool)
{
	addImportPool(importPool);
	addExportPool(exportPool);
}

ODF::Pool ODF::PoolCollection::makePool()
{
	return std::make_shared<PoolType>();
}

ODF::UnresolvedImport::UnresolvedImport(const std::string& message) : runtime_error(message) {}

ODF::VTypeInfo::TypeID::TypeID() : runtimeID(0), sss(0) {}

ODF::VTypeInfo::TypeID::TypeID(size_t runtimeID, unsigned char sss) : runtimeID(runtimeID), sss(sss) {}

void ODF::VTypeInfo::TypeID::saveToMemory(MemoryDataStream& mem) const
{
	saveToMemory(mem, runtimeID);
}

void ODF::VTypeInfo::TypeID::saveToMemory(MemoryDataStream& mem, size_t runtimeID) const
{
	std::pair id = PoolCollection::makeFullTypeID(runtimeID);
	switch (sss)
	{
	case 0: // uint8 and is treated as built-in type
		if (!id.second)
		{
			THROW InvalidTypeID();
		}
		mem.write<UINT_8>(id.first);
		break;
	case 1: // uint8
		mem.write<UINT_8>(id.first);
		break;
	case 2: // uint16
		mem.write<UINT_16>(id.first);
		break;
	case 3: // uint32
		mem.write<UINT_32>(id.first);
		break;
	}
}

unsigned char ODF::VTypeInfo::TypeID::addSSS(unsigned char typeSpec) const
{
	return (sss << 6) | typeSpec;
}

unsigned char ODF::VTypeInfo::TypeID::smallestSSS(size_t type)
{
	if (type <= UINT8_MAX)
		return 1;
	if (type <= UINT16_MAX)
		return 2;
	if (type <= UINT32_MAX)
		return 3;
	THROW InvalidTypeID("type id too large for calculating secondary size specifier");
}

void ODF::VTypeInfo::TypeID::matchSSS()
{
	sss = smallestSSS(runtimeID);
}

bool ODF::VTypeInfo::TypeID::expandsBuiltIn() const
{
	return (runtimeID & 0xFF00'0000'0000'0000) == 0xFF00'0000'0000'0000;
}

bool ODF::VTypeInfo::TypeID::operator==(const TypeID& other) const
{
	return (sss == other.sss || sss == WildcardSSS || other.sss == WildcardSSS)  && runtimeID == other.runtimeID;
}

void ODF::VTypeInfo::TypeID::operator++()
{
	runtimeID++;
}

void ODF::VTypeInfo::addExport(size_t id, unsigned char sss)
{
	if (exports.contains(id))
	{
		THROW VTypeDataAlreadyExists();
	}
	exports[id] = TypeID(id, sss);
}

void ODF::VTypeInfo::addExport(size_t localID, size_t globalID, unsigned char sss)
{
	if (exports.contains(localID))
	{
		THROW VTypeDataAlreadyExists();
	}
	exports[localID] = TypeID(globalID, sss);
}

void ODF::VTypeInfo::addImport(size_t id, unsigned char sss)
{
	if (imports.contains(id))
	{
		THROW VTypeDataAlreadyExists();
	}
	imports[id] = TypeID(id, sss);
}

void ODF::VTypeInfo::addImport(size_t globalID, size_t localID, unsigned char sss)
{
	if (imports.contains(globalID))
	{
		// ignore if the new and old value are equal
		if (imports.at(globalID) == TypeID(localID, sss))
			return;
		// else throw an exception
		THROW VTypeDataAlreadyExists();
	}
	imports[globalID] = TypeID(localID, sss);
}

ODF::VTypeInfo::TypeIDWithDependencies& ODF::VTypeInfo::addDefinedType(const Type& type, size_t typeID, unsigned char sss)
{
	if (definedTypes.contains(type))
	{
		THROW VTypeDataAlreadyExists();
	}
	return definedTypes[type] = TypeID(typeID, sss);
}

ODF::VTypeInfo::TypeIDWithDependencies& ODF::VTypeInfo::addDefinedType(const Type& type, const TypeIDWithDependencies& typeID)
{
	if (definedTypes.contains(type))
	{
		THROW VTypeDataAlreadyExists();
	}
	return definedTypes[type] = typeID;
}

ODF::VTypeInfo::TypeID ODF::VTypeInfo::getFreeTypeID() const
{
	unsigned int i = 0;
	do {
		TypeID id(i, TypeID::smallestSSS(i));
		if (!isAlreadyDefined(id))
			return id;
		i++;
	} while (i != UINT32_MAX);

	/// TODO
	//// first search through ids that replace built-in ones, as they are the most efficient.

	//int possibleSolutions = (31 - TypeSpecifier::LAST_SPEC) * 8;
	//size_t tries = 0;
	//unsigned char bio = 0;
	//while (true)
	//{
	//	bio = getNextBIOTypeID();
	//	tries++;
	//	if (tries > possibleSolutions)
	//		break;
	//	if (!isAlreadyDefined(PoolCollection::getFullTypeID(bio, true)))
	//		return PoolCollection::getFullTypeID(bio, true);
	//}

	//// search for full typeID
	//size_t id = 1; // start with 1, so the loop can terminate when tries reaches UINT64_MAX, just before it overflows.
	//tries = 0;
	//while (true)
	//{
	//	TypeID id(id, TypeID::smallestSSS(id));
	//	if (!isAlreadyDefined(id))
	//		return id;
	//	if (tries == UINT64_MAX)
	//		break;
	//}

	// no id was found
	THROW InvalidTypeID("InvalidTypeID: ODF::VTypeInfo::getFreeID() couldn't find any free typeID. This is bug if you didn't use 9.223.372.036.854.775.807*2+1 ids");
}

unsigned char ODF::VTypeInfo::getNextBIOTypeID(unsigned char type)
{
	do
		type++;
	while (TypeSpecifier::isBuiltIn(type));
	return type;
}

bool ODF::VTypeInfo::isAlreadyDefined(const Type& type) const
{
	return definedTypes.contains(type);
}

bool ODF::VTypeInfo::isAlreadyDefined(TypeID typeID) const
{
	for (const std::pair<Type, TypeID>& it : definedTypes)
		if (it.second == typeID)
			return true;
	return false;
}

std::optional<ODF::VTypeInfo::TypeID> ODF::VTypeInfo::getTypeID(const Type& type) const
{
	try
	{
		return definedTypes.at(type);
	}
	catch (std::out_of_range&)
	{
		return std::nullopt;
	}
}

bool ODF::VTypeInfo::useType(MemoryDataStream& mem, const Type& type, const ConstParseInfo& parseInfo) const
{
	try
	{
		const TypeID* id = reinterpret_cast<const TypeID*>(&parseInfo.vtypeinfo->definedTypes.at(type)); // try to access the id before writing anything to the stream to prevent writing of USETYPE in case of exception. Use reinterpret_cast and poitner to prevent slicing
		if (!id->expandsBuiltIn())
			mem.write(id->addSSS(TypeSpecifier::USETYPE));
		id->saveToMemory(mem);
		return true;
	}
	catch (std::out_of_range&)
	{
		// in case the type isn't already defined, it will be defined if noDefType is false		
		if (parseInfo.flags & ConstParseInfo::FLAG_NO_IMPLICIT_DEFTYPE)
		{
			ConstParseInfo localParseInfo = parseInfo;
			localParseInfo.flags |= ConstParseInfo::FLAG_NO_USETYPE; // avoid calling the current function again from saveToMemory, as this would lead to a StackOverflow exception.
			type.saveToMemory(mem, localParseInfo); // save type
		}
		else
		{
			// create a TypeIDTypeID newID;
			TypeID newID = getFreeTypeID();

			// register Type
			const_cast<VTypeInfo*>(this)->addDefinedType(type, newID); // sus because const cast. Change this maybe? TODO change this to mutable

			// define the type in the file
			mem.write(newID.addSSS(TypeSpecifier::DEFTYPE));
			newID.saveToMemory(mem); // save id
			type.saveToMemory(mem, parseInfo); // save type

			// add USETYPE so it is available as if nothing happened
			mem.write(newID.addSSS(TypeSpecifier::USETYPE));
			newID.saveToMemory(mem);
		}
		
		return false;
	}
}

void ODF::VTypeInfo::saveDefinedTypes(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const
{
	ConstParseInfo localParseInfo = parseInfo; // do no edit the passed parseInfo
	localParseInfo.flags |= ConstParseInfo::FLAG_NO_USETYPE; // as all existing types are defined here, USETYPE isn't allowed
	//localParseInfo.flags |= ConstParseInfo::FLAG_NO_IMPLICIT_DEFTYPE;

	// a little explanation for the following algorithm:
	// alreadySaved cotains all types that already have been saved.
	// THen the the function will iterate over the types that needs to be saved. If the dependencies are met, the type will be saved, otherwise it will be skipped.
	// Because the iteration for loop is in a loop itself, eventually the programm will encounter it again after the dependencies are saved, and save it.
	// If a circular dependency occurs, eventually the loop will come to a state, where it won't save anything because both types are waiting for each other.
	// This will be detected by check if in one iteration of the for-loop nothing was saved. If this is true, the function will crash.

	std::vector<const TypeID*> alreadySaved;
	alreadySaved.reserve(definedTypes.size());

	std::vector<const std::pair<const Type, TypeIDWithDependencies>*> leftToSave;
	leftToSave.reserve(definedTypes.size());
	for (const auto& it : definedTypes)
		leftToSave.push_back(&it);

	while (alreadySaved.size() != definedTypes.size())
	{
		bool nothingSaved = true; // for detecting circular dependencies
		for (const std::pair<const Type, TypeIDWithDependencies>*& it : leftToSave)
		{
			// skip if already saved
			if (!it)
				continue;

			// check dependencies
			for (const TypeID& typeIt : it->second.dependencies)
				if (std::ranges::find(alreadySaved, typeIt, [](const TypeID* tid) {return *tid; }) == alreadySaved.end()) // a dependency wasn't found
					goto skip_iteration; // break or continue wouldn't work because of the inner for loop

			// save
			mem.write(it->second.addSSS(TypeSpecifier::DEFTYPE));
			it->second.saveToMemory(mem);
			it->first.saveToMemory(mem, localParseInfo);
			nothingSaved = false;

			// make it avaiable as dependency
			alreadySaved.push_back(reinterpret_cast<const TypeID*>(&it->second));

			// remove from leftToSave
			it = nullptr;

		skip_iteration:
			; // syntax error "}" without this
		}
	}
}

void ODF::VTypeInfo::saveImports(MemoryDataStream& mem) const
{
	// use IMPORTAS globalID (key) localID (value) if key and value are not the same. Otherwise use IMPORT ID
	for (const std::pair<size_t, TypeID>& it : imports)
	{
		if (it.first == it.second.runtimeID)
		{ // IMPORT
			mem.write(it.second.addSSS(TypeSpecifier::IMPORT));
			it.second.saveToMemory(mem, it.first); // save the typeID it.first with the sss from it.second
		}
		else
		{ // IMPORTAS
			mem.write(it.second.addSSS(TypeSpecifier::IMPORTAS));
			it.second.saveToMemory(mem, it.first); // save the typeID it.first with the sss from it.second
			it.second.saveToMemory(mem); // short for it.second.saveToMemory(mem, it.second);
		}
	}
}

void ODF::VTypeInfo::saveExports(MemoryDataStream& mem) const
{
	for (const std::pair<size_t, TypeID>& it : exports)
	{
		if (it.first == it.second.runtimeID)
		{ // EXPORT
			mem.write(it.second.addSSS(TypeSpecifier::EXPORT));
			it.second.saveToMemory(mem, it.first); // save the typeID it.first with the sss from it.second
		}
		else
		{ // EXPORTAS
			mem.write(it.second.addSSS(TypeSpecifier::EXPORTAS));
			it.second.saveToMemory(mem, it.first); // save the typeID it.first with the sss from it.second
			it.second.saveToMemory(mem); // short for it.second.saveToMemory(mem, it.second);
		}
	}
}

void ODF::VTypeInfo::saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const
{
	saveDefinedTypes(mem, parseInfo);
	saveImports(mem); // imports are saved before the exports in case an imported type is exported again (which wouldn't make much sense, though it is not explicitely prohibited)
	saveExports(mem);
}

ODF::VTypeInfo::operator bool()
{
	return definedTypes.size() || imports.size() || exports.size();
}

std::optional<ODF::Type> ODF::ParseInfo::importType(size_t id) const
{
	return importType(PoolCollection::TypeID(id, 0));
}

std::optional<ODF::Type> ODF::ParseInfo::importType(ODF::PoolCollection::TypeID id) const
{
	auto it = localPool->find(id);
	if (it != localPool->end())
		return it->second;
	pools->importType(id);
}

bool ODF::ParseInfo::containsInvalidPointer() const
{
	return !pools || !localPool || !vtypeinfo;
}

std::shared_ptr<ODF::PoolType>& ODF::ParseInfo::guarateeLocalPool()
{
	if (!localPool)
		localPool = std::make_shared<PoolType>();
	return localPool;
}

std::shared_ptr<ODF::PoolCollection>& ODF::ParseInfo::guaranteePoolCollection()
{
	if (!pools)
		pools = std::make_shared<PoolCollection>();
	return pools;
}

ODF::ParseInfo::ParseInfo(std::shared_ptr<PoolCollection>& pools, std::shared_ptr<PoolType>& localPool, std::shared_ptr<VTypeInfo>& vtypeinfo) : dependecies(nullptr)
{
	this->pools = pools ? pools : pools = std::make_shared<PoolCollection>(); // assigns to pools if pools holds a value, otherwise points both pools pointer to a new PoolCollection
	this->localPool = localPool ? localPool : localPool = std::make_shared<PoolType>();
	this->vtypeinfo = vtypeinfo ? vtypeinfo : vtypeinfo = std::make_shared<VTypeInfo>();
}

ODF::ParseInfo::ParseInfo(const std::shared_ptr<PoolCollection>& pools, const std::shared_ptr<PoolType>& localPool, const std::shared_ptr<VTypeInfo>& vtypeinfo) : dependecies(nullptr)
{
	if (!pools)
	{
		THROW InvalidParseInformation("InvalidParseInformation: ParseInfo::ParseInfo(...) with 'const std::shared_ptr<PoolCollection>& pools' was called, though pools does not contain a valid value");
	}
	if (!localPool)
	{
		THROW InvalidParseInformation("InvalidParseInformation: ParseInfo::ParseInfo(...) with 'const std::shared_ptr<PoolCollection>& pools' was called, though localPool does not contain a valid value");
	}
	if (!vtypeinfo)
	{
		THROW InvalidParseInformation("InvalidParseInformation: ParseInfo::ParseInfo(...) with 'const std::shared_ptr<PoolCollection>& pools' was called, though vtypeinfo does not contain a valid value");
	}
	this->pools = pools;
	this->localPool = localPool;
	this->vtypeinfo = vtypeinfo;
}

ODF::ConstParseInfo::ConstParseInfo(const std::shared_ptr<const PoolCollection>& pools, const std::shared_ptr<const PoolType>& localPool, const std::shared_ptr<const VTypeInfo>& vtypeinfo) : pools(pools), localPool(localPool), vtypeinfo(vtypeinfo), flags(0) {}

ODF::InvalidParseInformation::InvalidParseInformation(const std::string& message) : runtime_error(message) {}

ODF::VTypeDataAlreadyExists::VTypeDataAlreadyExists(const std::string& message) : runtime_error(message) {}


// little helper function for ODF::TypeHasher::operator()
size_t hash_combine(size_t lhs, size_t rhs) {
	if constexpr (sizeof(size_t) >= 8) {
		lhs ^= rhs + 0x517cc1b727220a95 + (lhs << 6) + (lhs >> 2);
	}
	else {
		lhs ^= rhs + 0x9e3779b9 + (lhs << 6) + (lhs >> 2);
	}
	return lhs;
}

size_t ODF::TypeHasher::operator()(const Type& type) const
{
	// properties that needs to be hashed are complexSpec, immutable and type
	size_t complexSpecHash = type.complexSpec ? ComplexSpecifierHasher{}(*type.complexSpec) : 0;
	size_t immutableHash = std::hash<bool>{}(type.immutable);
	size_t typeHash = std::hash<unsigned char>{}(type.type);
	return hash_combine(complexSpecHash, hash_combine(immutableHash, typeHash));
}

size_t ODF::VTypeInfo::TypeIDHasher::operator()(const TypeID& id) const
{
	return std::hash<size_t>{}(id.runtimeID);
}

ODF::InvalidTypeID::InvalidTypeID(const std::string& message) : runtime_error(message) {}

void ODF::PrettyPrintInfo::pre(std::ostream& out)
{
	for (int i = 0; i < indent; i++)
	{
		if (i == indent - 1)
			out << "+--";
		else
			out << "|  ";
	}
}

void ODF::PrettyPrintInfo::post(std::ostream& out)
{
}

ODF::PrettyPrintInfo::PrettyPrintInfo() : indent(0)
{
}

ODF::VTypeInfo::TypeIDWithDependencies::TypeIDWithDependencies(const TypeID& other)
{
	runtimeID = other.runtimeID;
	sss = other.sss;
}

ODF::VTypeInfo::TypeIDWithDependencies& ODF::VTypeInfo::TypeIDWithDependencies::operator=(const TypeID& tid)
{
	runtimeID = tid.runtimeID;
	sss = tid.sss;
	return *this;
}

size_t ODF::ComplexSpecifierHasher::operator()(const Type::ComplexSpecifier& cs)
{
	size_t hash = std::hash<size_t>{}(cs.index());
	if (auto ptr = std::get_if<ObjectSpecifier>(&cs))
		return hash_combine(hash, ObjectSpecifierHasher{}(*ptr));
	else
		return hash_combine(hash, ArraySpecifierHasher{}(std::get<ArraySpecifier>(cs)));
}

size_t ODF::FixedArraySpecifierHasher::operator()(const FixedArraySpecifier& farrSpec)
{
	return hash_combine(std::hash<size_t>{}(farrSpec.size.actualSize), TypeHasher{}(farrSpec.fixType));
}

size_t ODF::MixedArraySpecifierHasher::operator()(const MixedArraySpecifier& marrSpec)
{
	if (marrSpec.types.empty())
		return 0;
	size_t hash = TypeHasher{}(marrSpec.types[0]);
	for (size_t i = 1; i < marrSpec.types.size(); i++)
		hash = hash_combine(hash, TypeHasher{}(marrSpec.types[i]));
	return hash;
}

size_t ODF::FixedObjectSpecifierHasher::operator()(const FixedObjectSpecifier& fobjSpec)
{
	size_t hash = TypeHasher{}(fobjSpec.fixType);
	for (const std::string& key : fobjSpec.keys)
		hash = hash_combine(hash, std::hash<std::string>{}(key));
	return hash;
}

size_t ODF::MixedObjectSpecifierHasher::operator()(const MixedObjectSpecifier& mobjSpec)
{
	// combine all keys and all type hashes by XOR and the hash_combine them, to the order of the properties doesn't matter
	size_t keyHash = 0, typeHash = 0;
	for (const auto& it : mobjSpec.properties)
	{
		keyHash ^= std::hash<std::string>{}(it.first);
		typeHash ^= TypeHasher{}(it.second);
	}
	return hash_combine(keyHash, typeHash);
}

size_t ODF::ObjectSpecifierHasher::operator()(const ObjectSpecifier& objSpec)
{
	size_t hash = std::hash<size_t>{}(objSpec.index());
	if (auto ptr = std::get_if<MixedObjectSpecifier>(&objSpec))
		return hash_combine(hash, MixedObjectSpecifierHasher{}(*ptr));
	else
		return hash_combine(hash, FixedObjectSpecifierHasher{}(std::get<FixedObjectSpecifier>(objSpec)));
}

size_t ODF::ArraySpecifierHasher::operator()(const ArraySpecifier& arrSpec)
{
	size_t hash = std::hash<size_t>{}(arrSpec.index());
	if (auto ptr = std::get_if<MixedArraySpecifier>(&arrSpec))
		return hash_combine(hash, MixedArraySpecifierHasher{}(*ptr));
	else
		return hash_combine(hash, FixedArraySpecifierHasher{}(std::get<FixedArraySpecifier>(arrSpec)));
}
