#include "ODF.h"

bool ODF::Config::PreventAutomaticMergeLossFloat = true;
bool ODF::Config::PreventAutomaticMergeLossString = true;
unsigned char ODF::Config::DefaultIntegerMergeSize = 0;

#pragma region specifiers
ODF::Status ODF::Type::saveToMemory(MemoryDataStream& mem) const
{
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

	if (!obj || !list)
		return Status::Ok;

	if (list)
	{
		ArraySpecifier& listSpec = std::get<ArraySpecifier>(*complexSpec);
		if (fixed)
		{
			FixedArraySpecifier& flistSpec = std::get<FixedArraySpecifier>(listSpec);
			flistSpec.saveToMemory(mem);
			return Status::Ok;
		}
		else if (mixed)
		{
			MixedArraySpecifier& mlistSpec = std::get<MixedArraySpecifier>(listSpec);
			mlistSpec.saveToMemory(mem);
			return Status::Ok;
		}
	}
	else if (obj)
	{
		ObjectSpecifier& objSpec = std::get<ObjectSpecifier>(*complexSpec);
		if (fixed)
		{
			FixedObjectSpecifier& fobjSpec = std::get<FixedObjectSpecifier>(objSpec);
			fobjSpec.saveToMemory(mem);
			return Status::Ok;
		}
		else if (mixed)
		{
			MixedObjectSpecifier& mobjSpec = std::get<MixedObjectSpecifier>(objSpec);
			mobjSpec.saveToMemory(mem);
			return Status::Ok;
		}
	}

	return Status::Ok;
}

ODF::Status ODF::Type::loadFromMemory(MemoryDataStream& mem)
{
	destroyComplexSpec(); // destroy the last complexSpec if the object was already in use

	// load primitive type specifier, to allow isFixed() check
	type = mem.read<unsigned char>(); // prio 1
	bool obj = type.isObject();
	bool list = type.isList();
	bool fixed = type.isFixed();
	bool mixed = type.isMixed();

	if (!obj || !list)
		return Status::Ok;

	makeComplexSpec(); // make sure there's a valid complexSpec

	// load individual complex types
	if (list)
	{
		ArraySpecifier& listSpec = complexSpec->emplace<ArraySpecifier>();
		if (fixed)
		{
			FixedArraySpecifier& flistSpec = listSpec.emplace<FixedArraySpecifier>();
			flistSpec.loadFromMemory(mem);
			return Status::Ok;
		}
		else if (mixed)
		{
			MixedArraySpecifier& mlistSpec = listSpec.emplace<MixedArraySpecifier>();
			mlistSpec.loadFromMemory(mem);
			return Status::Ok;
		}
	}
	else if (obj)
	{
		ObjectSpecifier& objSpec = complexSpec->emplace<ObjectSpecifier>();
		if (fixed)
		{
			FixedObjectSpecifier& fobjSpec = objSpec.emplace<FixedObjectSpecifier>();
			fobjSpec.loadFromMemory(mem); // already handles all loading
			return Status::Ok;
		}
		else if (mixed)
		{
			MixedObjectSpecifier& mobjSpec = objSpec.emplace<MixedObjectSpecifier>();
			mobjSpec.loadFromMemory(mem);
			return Status::Ok;
		}
	}

	return Status::Ok;
}

#pragma region TypeSpecifier
inline ODF::TypeSpecifier::Type ODF::TypeSpecifier::byte() const
{
	return type;
}

inline ODF::TypeSpecifier::Type ODF::TypeSpecifier::sizeSpec() const
{
	return type & 0b11000000;
}

inline bool ODF::TypeSpecifier::isSigned() const
{
	return !(type & 0b00100000);
}

inline bool ODF::TypeSpecifier::isUnsigned() const
{
	return type & 0b00100000;
}

ODF::TypeSpecifier::operator Type() const
{
	// without size spec
	return type & 0b00111111;
}

ODF::TypeSpecifier& ODF::TypeSpecifier::operator=(unsigned char byte)
{
	// TODO: hier return-Anweisung eingeben
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

std::ostream& operator<<(std::ostream& out, const ODF& odf)
{
	if (auto ptr = std::get_if<INT_8>(&odf.content)) { out << (odf.printType ? "[BYTE] " : "") << *ptr; }
	if (auto ptr = std::get_if<UINT_8>(&odf.content)) { out << (odf.printType ? "[UBYTE] " : "") << *ptr; }
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
	if (auto ptr = std::get_if<ODF::Object>(&odf.content)) { out << (odf.printType ? "[OBJ] " : "") << *ptr; }
	if (auto ptr = std::get_if<ODF::Array>(&odf.content)) { out << (odf.printType ? "[LIST] " : "") << *ptr; }
	return out;
}

std::ostream& operator<<(std::ostream& out, const ODF::Object& obj)
{
	out << "[Object print not implemented]";
	return out;
}

std::ostream& operator<<(std::ostream& out, const ODF::Array& list)
{
	if (list.isMixed())
	{
		out << "<MX>\n";
		size_t i = 0;
		for (const ODF& odf : const_cast<ODF::Array&>(list).getIterationContainer()) // bruh
			out << i++ << ": " << odf << "\n";
	}
	else
	{
		out << "<FX(" << list.getFixedDatatype() << ")>\n";
		size_t i = 0;
		for (ODF& odf : const_cast<ODF::Array&>(list).getIterationContainer()) // bruh
		{
			bool printType = odf.printType;
			odf.printType = false;
			out << i++ << ": " << odf << "\n";
			odf.printType = printType;
		}
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

bool operator==(const ODF& odf1, const ODF& odf2)
{
	return false;
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
	switch (type)
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
	case TS::FXLIST: case TS::MXLIST: return VT_OBJ;
	case TS::FXOBJ: case TS::MXOBJ: return VT_OBJ;
	default: THROW VariantTypeConversionError();
	}
}

void ODF::Type::destroyComplexSpec()
{
	if (complexSpec)
		delete complexSpec;
	complexSpec = nullptr;
}

void ODF::Type::makeComplexSpec()
{
	if (!complexSpec)
		complexSpec = new ComplexSpecifier;
}

void ODF::Type::resetComplexSpec()
{
	destroyComplexSpec();
	complexSpec = new ComplexSpecifier;
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

ODF ODF::getAs(TypeSpecifier spec) const
{
	return ODF();
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

ODF& ODF::operator=(const std::wstring& wstr)
{
	type = TypeSpecifier::WSTR;
	content = wstr;
	return *this;
}

ODF& ODF::operator=(const Object& obj)
{
	type = obj.isMixed() ? TypeSpecifier::MXOBJ : TypeSpecifier::FXOBJ;
	content = obj;
	return *this;
}

ODF& ODF::operator=(const Array& arr)
{
	type = arr.isMixed() ? TypeSpecifier::MXLIST : TypeSpecifier::FXLIST;
	content = arr;
	return *this;
}

ODF& ODF::operator=(const Array::MixedArray& marr)
{
	return *this = Array(marr);
}

ODF& ODF::operator=(const std::initializer_list<ODF>& initializer_list)
{
	return *this = Array(initializer_list);
}

ODF& ODF::operator=(const Array::FixedArray& farr)
{
	return *this = Array(farr);
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

ODF::operator Array()
{
	return std::get<Array>(content);
}

ODF::ODF() : printType(true)
{
}

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

ODF::ODF(const Array& arr) : ODF()
{
	*this = arr;
}

ODF::ODF(const Array::MixedArray& marr) : ODF(Array(marr)) {}

ODF::ODF(const Array::FixedArray& farr) : ODF(Array(farr)) {}

ODF::ODF(const std::initializer_list<ODF>& initializer_list) : ODF(Array(initializer_list)) {}

void ODF::makeList(bool fixed)
{
	content.emplace<Array>();
	Array& list = std::get<Array>(content);
	if (fixed)
	{
		type = TypeSpecifier::FXLIST;
		list.clearAndFix();
	}
	else
	{
		type = TypeSpecifier::MXLIST;
		list.clearAndMix();
	}
}

void ODF::makeList(const Type& elementType)
{
	content.emplace<Array>();
	Array& list = std::get<Array>(content);
	type = TypeSpecifier::FXLIST;
	list.clearAndFix(elementType);
}

ODF::Type::operator unsigned char()
{
	return type;
}

ODF::Type& ODF::Type::operator=(const Type& other)
{
	if (&other == this)
		return *this;

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
	setType(type);
	return type;
}

ODF::Type::Type()
{
	complexSpec = nullptr;
}

ODF::Type::Type(const Type& other)
{
	complexSpec = nullptr;
	*this = other;
}

ODF::Type::~Type()
{
}

#pragma endregion
#pragma region ObjectSpecifier
void ODF::MixedObjectSpecifier::saveToMemory(MemoryDataStream& mem) const
{
	// CSTR key
	// TypeHeader value
	// empty key (just null terminator) = end of specifier
	for (ObjectIterator it : properties)
	{
		// save key
		mem.writeStr(it.first);
		// save type
		it.second.saveToMemory(mem);
	}
	// save zero to end object specifier
	mem.write<UINT_8>(0);
}

void ODF::MixedObjectSpecifier::loadFromMemory(MemoryDataStream& mem)
{
	properties.clear();
	std::string key;
	
	while (mem.peek())
	{
		key = mem.readStr();
		Type type;
		type.loadFromMemory(mem);
		properties[key] = type;
	}
}

void ODF::FixedObjectSpecifier::saveToMemory(MemoryDataStream& mem) const
{
	// format:
	// TYPE type
	// CSTR key1
	// CSTR key2
	// ...
	// empty key = end

	if (!fixType)
		THROW 0; // TODO

	// save type
	fixType->saveToMemory(mem);
	// save keys
	for (const std::string& key : keys)
		mem.writeStr(key);
	// save zero to end specifier
	mem.write<UINT_8>(0);
}

void ODF::FixedObjectSpecifier::loadFromMemory(MemoryDataStream& mem)
{
	// prepare object for reading
	keys.clear();
	if (fixType)
		delete fixType;
	fixType = new Type;
	
	// load header
	fixType->loadFromMemory(mem);

	// read keys until a zero terminator
	while (mem.peek())
		keys.push_back(mem.readStr());
}

ODF::FixedObjectSpecifier::FixedObjectSpecifier() : fixType(nullptr) {}

ODF::FixedObjectSpecifier::~FixedObjectSpecifier()
{
	if (fixType)
		delete fixType;
}

#pragma endregion
#pragma region SizeSpecifier
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
	switch (type % 4)
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

void ODF::SizeSpecifier::saveToMemory(MemoryDataStream& mem) const
{
	// modify sizeSpecSpec in type
	mem.setPrevious(sizeSpecifierSpecifier(mem.peekPrevious()));
	saveToMemory(mem, sizeSpecifierSpecifier());
}

void ODF::SizeSpecifier::loadFromMemory(MemoryDataStream& mem)
{
	load(mem, mem.peekPrevious());
}

#pragma endregion
#pragma endregion

ODF::Status ODF::saveToMemory(MemoryDataStream& mem) const
{
	// save header
	if (Status error = type.saveToMemory(mem))
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
	if (Status error = type.loadFromMemory(mem))
		return error;

	return loadBody(mem);
}

void ODF::Type::setType(TypeSpecifier type)
{
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

inline bool ODF::TypeSpecifier::isMixed() const
{
	return type & FLAG_MIXED;
}

inline bool ODF::TypeSpecifier::isFixed() const
{
	// if type is list or object and FLAG_MIXED is not set
	return (type & FLAG_OBJ || type & FLAG_LIST) && !(type & FLAG_MIXED);
}

inline bool ODF::TypeSpecifier::isObject() const
{
	return smallType() == FLAG_OBJ;
}

inline bool ODF::TypeSpecifier::isList() const
{
	return type & FLAG_LIST;
}

inline ODF::TypeSpecifier::Type ODF::TypeSpecifier::smallType() const
{
	return type & 0b0001'1111;
}

inline ODF::TypeSpecifier::Type ODF::TypeSpecifier::withoutSSS() const
{
	return type & 0b00'111111;
}

void ODF::TypeSpecifier::saveToMemory(MemoryDataStream& mem) const
{
	mem.write(type);
}

void ODF::TypeSpecifier::loadFromMemory(MemoryDataStream& mem)
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

ODF::Status ODF::saveToFile(std::string file)
{
	return Status::Ok;
}

ODF::Status ODF::loadFromFile(std::string file)
{
	return Status::Ok;
}

ODF::Status ODF::saveToStream(std::ostream& out, bool binary)
{
	return Status::Ok;
}

ODF::Status ODF::loadFromStream(std::istream& in, bool binary)
{
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
	catch (std::bad_variant_access& exp)
	{
		return Status::TypeMismatch;
	}
}

ODF::Status ODF::saveObject(MemoryDataStream& mem) const
{
	return Status();
}

ODF::Status ODF::saveArray(MemoryDataStream& mem) const
{
	return Status();
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
	return Status();
}

ODF::Status ODF::loadArray(MemoryDataStream& mem)
{
	return Status();
}

ODF::Status::operator bool()
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

bool ODF::Status::operator==(Status other)
{
	return content == other.content;
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

void ODF::AbstractArray::push_back(const ODF& odf)
{
	list.push_back(odf);
}

void ODF::push_back(const ODF& odf)
{
	std::get<Array>(content).push_back(odf);
}

void ODF::push_front(const ODF& odf)
{
	std::get<Array>(content).push_front(odf);
}

void ODF::erase(size_t index)
{
	std::get<Array>(content).erase(index);
}

void ODF::erase(size_t index, size_t numberOfElements)
{
	std::get<Array>(content).erase(index, numberOfElements);
}

void ODF::erase_range(size_t from, size_t to)
{
	std::get<Array>(content).erase_range(from, to);
}

void ODF::insert(size_t where, const ODF& odf)
{
	std::get<Array>(content).insert(where, odf);
}

size_t ODF::find(const ODF& odf)
{
	return std::get<Array>(content).find(odf);
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

size_t ODF::find_nothrow(const ODF& odf) noexcept
{
	try {
		return find(odf);
	}
	catch (...)
	{
		return (size_t)-1;
	}
}

void ODF::AbstractArray::push_front(const ODF& odf)
{
	list.insert(list.begin(), odf);
}

void ODF::AbstractArray::erase(size_t index)
{
	list.erase(list.begin() + index);
}

void ODF::AbstractArray::erase(size_t index, size_t numberOfElements)
{
	list.erase(list.begin() + index, list.begin() + index + numberOfElements - 1);
}

void ODF::AbstractArray::erase_range(size_t from, size_t to)
{
	list.erase(list.begin() + from, list.begin() + to);
}

void ODF::AbstractArray::insert(size_t where, const ODF& odf)
{
	list.insert(list.begin() + where, odf);
}

size_t ODF::AbstractArray::find(const ODF& odf)
{
	// do a linear search
	for (size_t i = 0; i < list.size(); i++)
		if (list[i] == odf)
			return i;
	return -1; // corresponds to UINT64_MAX
}

size_t ODF::AbstractArray::size() const
{
	return list.size();
}

ODF& ODF::AbstractArray::operator[](size_t index)
{
	return list[index];
}

const ODF& ODF::AbstractArray::operator[](size_t index) const
{
	return list[index];
}

std::vector<ODF>& ODF::AbstractArray::getIterationContainer()
{
	return list;
}

void ODF::Array::FixedArray::push_back(const ODF& odf)
{
	// check type, then push_back, otherwise register fail
	Type realType = odf.getComplexType();
	if (fixType.type == TypeSpecifier::NULLTYPE)
		fixType = realType; // trigger next if statement. COmpiler will propably optimaize this out
	if (realType == fixType)
		list.push_back(odf);
	else if (fails)
		fails->push_back(&odf);
	else
	{
		THROW std::bad_variant_access();
	}
}

void ODF::Array::FixedArray::push_front(const ODF& odf)
{
	// check type, then push_back, otherwise register fail
	if (odf.getComplexType() == fixType)
		list.insert(list.begin(), odf);
	else if (fails)
		fails->push_back(&odf);
}

void ODF::Array::FixedArray::insert(size_t where, const ODF& odf)
{
	// check type, then push_back, otherwise register fail
	if (odf.getComplexType() == fixType)
		list.insert(list.begin() + where, odf);
	else if (fails)
		fails->push_back(&odf);
}

void ODF::Array::FixedArray::setType(const Type& type)
{
}

inline const ODF::Type& ODF::Array::FixedArray::getType() const
{
	return fixType;
}

inline void ODF::Array::FixedArray::enableFailRegistry(bool enable)
{
}

bool ODF::Array::FixedArray::fail() const
{
	return false; // TODO
}

inline const std::vector<const ODF*>& ODF::Array::FixedArray::getFails()
{
	// TODO: hier return-Anweisung eingeben
	std::vector<const ODF*> TODO;
	return TODO;
}

ODF::Array::FixedArray::FixedArray() : fails(nullptr)
{
}

bool ODF::Array::MixedArray::canBeFixed(FixInfo* info) const
{
	// check for same type, as this is the default case where the caller already constructed a valid Fixed List inside a Mixed List
	if (!list.size()) // empty lists cannot be fixed, as no Type is provided.
		return false;
	Type complexType = list[0].getComplexType();
	bool match = true;
	for (size_t i = 1; i < list.size(); i++)
		if (list[i].getComplexType() != complexType)
		{
			match = false;
			break;
		}
	if (match)
	{
		if (info)
		{
			info->tcmatch = false;
			info->targetType = complexType;
		}
		return true;
	}

	// check for an integer or float type match
	TypeClass tc = list[0].getTypeClass();
	if (tc == TypeClass::Other)
		return false;

	// check whole list and get max precision
	for (size_t i = 1; i < list.size(); i++)
		if (list[i].getTypeClass() != tc)
			return false;

	// no match possible
	if (info)
	{
		info->tcmatch = true;
		info->tc = tc;
	}
	return true;
}

ODF::Array::FixedArray* ODF::Array::MixedArray::tryFixing(FixInfo* info)
{
	if (!info)
	{
		if (!canBeFixed(info))
			return nullptr;
	}

	// convert to mixed
	FixedArray* result = new FixedArray;

	if (info->tcmatch)
	{ // match only be type class
		unsigned char maxPrecision = 0;
		for (const ODF& odf : list)
			if (unsigned char precision = odf.getPrecision() > maxPrecision)
				maxPrecision = precision;

		// convert everything to the new precision
		info->targetType.type = getTypeFromTC(info->tc, maxPrecision);
		result->setType(info->targetType);

		// tagettype is primitive becauuse of tcmatch
		for (const ODF& odf : list)
			result->push_back(odf.getAs(info->targetType.type));
		return result;
	}

	result->enableFailRegistry();
	result->setType(info->targetType);
	for (const ODF& odf : list)
		result->push_back(odf);
	if (result->fail())
	{
		std::cout << "ODF error: conversion to fixed failed: " << result->getFails().size() << " Subconversions failed.\n";
		delete result;
		return nullptr;
	}

	result->enableFailRegistry(false);
	return result;
}

ODF::Array::MixedArray::MixedArray()
{
}

ODF::Array::MixedArray::MixedArray(const std::initializer_list<ODF>& initializer_list)
{
}

ODF::Array::FixedArray::InvalidTypeChange::InvalidTypeChange(std::string message) : runtime_error(message)
{
}

void ODF::Object::makeMixed() const
{
}

bool ODF::Object::canBeFixed() const
{
	return false;
}

bool ODF::Object::tryFixing() const
{
	return false;
}

bool ODF::Object::isMixed() const
{
	return false;
}

ODF::Type ODF::Object::getFixedDatatype() const
{
	return Type();
}

ODF::Type ODF::Object::getTargetDatatype() const
{
	return Type();
}

void ODF::Array::clearAndFix()
{
	list.emplace<FixedArray>();
}

void ODF::Array::clearAndFix(const Type& elementType)
{
	list.emplace<FixedArray>();
	std::get<FixedArray>(list).setType(elementType);
}

void ODF::Array::clearAndMix()
{
	list.emplace<MixedArray>();
}

void ODF::Array::makeMixed() const
{
}

bool ODF::Array::canBeFixed() const
{
	return false;
}

bool ODF::Array::tryFixing() const
{
	return false;
}

bool ODF::Array::isMixed() const
{
	return list.index();
}

ODF::Type ODF::Array::getFixedDatatype() const
{
	return std::get<FixedArray>(list).getType();
}

ODF::Type ODF::Array::getTargetDatatype() const
{
	return Type();
}

void ODF::Array::push_back(const ODF& odf)
{
	std::visit([&odf](ODF::AbstractArray& base) {
		base.push_back(odf);
		}, list);
}

void ODF::Array::push_front(const ODF& odf)
{
	std::visit([&odf](ODF::AbstractArray& base) {
		base.push_front(odf);
		}, list);
}

void ODF::Array::erase(size_t index)
{
	std::visit([&index](ODF::AbstractArray& base) {
		base.erase(index);
		}, list);
}

void ODF::Array::erase(size_t index, size_t numberOfElements)
{
	std::visit([&](ODF::AbstractArray& base) {
		base.erase(index, numberOfElements);
		}, list);
}

void ODF::Array::erase_range(size_t from, size_t to)
{
	std::visit([&](ODF::AbstractArray& base) {
		base.erase_range(from, to);
		}, list);
}

void ODF::Array::insert(size_t where, const ODF& odf)
{
	std::visit([&](ODF::AbstractArray& base) {
		base.insert(where, odf);
		}, list);
}

size_t ODF::Array::find(const ODF& odf)
{
	return std::visit([&odf](ODF::AbstractArray& base) -> size_t {
		return base.find(odf);
		}, list);
}

size_t ODF::Array::size() const
{
	return std::visit([](const ODF::AbstractArray& base) {
		return base.size();
		}, list);
}

ODF& ODF::Array::operator[](size_t index)
{
	return std::visit([&](AbstractArray& base) -> ODF& { return base[index]; }, list);
}

const ODF& ODF::Array::operator[](size_t index) const
{
	return std::visit([&](const AbstractArray& base) -> const ODF& { return base[index]; }, list);
}

std::vector<ODF>& ODF::Array::getIterationContainer()
{
	if (auto ptr = std::get_if<MixedArray>(&list))
		return ptr->getIterationContainer();
	else if (auto ptr = std::get_if<FixedArray>(&list))
		return ptr->getIterationContainer();
	else
		return *(std::vector<ODF>*)nullptr; // should never be reached. Fuck this.
}

ODF::Array& ODF::Array::operator=(const Array& other)
{
	if (&other == this)
		return *this;

	list = other.list;
	return *this;
}

ODF::Array& ODF::Array::operator=(const MixedArray& other)
{
	list = other;
	return *this;
}

ODF::Array& ODF::Array::operator=(const std::initializer_list<ODF>& initializer_list)
{
	*this = MixedArray(initializer_list);
	return *this;
}

ODF::Array& ODF::Array::operator=(const FixedArray& other)
{
	list = other;
	return *this;
}

ODF::Array::Array()
{
}

ODF::Array::Array(const Array& arr) : Array()
{
	*this = arr;
}

ODF::Array::Array(const MixedArray& marr) : Array()
{
	*this = marr;
}

ODF::Array::Array(const std::initializer_list<ODF>& initializer_list) : Array()
{
	*this = initializer_list;
}

ODF::Array::Array(const FixedArray& farr) : Array()
{
	*this = farr;
}

ODF::VariantTypeConversionError::VariantTypeConversionError(std::string message) : runtime_error(message)
{
}

void ODF::FixedArraySpecifier::saveToMemory(MemoryDataStream& mem) const
{
	size.saveToMemory(mem);
}

void ODF::FixedArraySpecifier::loadFromMemory(MemoryDataStream& mem)
{
	size.loadFromMemory(mem);
	fixType->loadFromMemory(mem);
}

ODF::SixSeven::SixSeven(const std::string& message) : runtime_error(message) {}

ODF::TypeMismatch::TypeMismatch(const std::string& message) : runtime_error(message) {}

void ODF::MixedArraySpecifier::saveToMemory(MemoryDataStream& mem) const
{
	for (const Type& type : types)
		type.saveToMemory(mem);
	mem.write(0ui8); // NULLTYPE
}

void ODF::MixedArraySpecifier::loadFromMemory(MemoryDataStream& mem)
{
	// load types until a NULL type is found
	while (mem.peek()) // next read type is not NULL
	{
		Type type;
		type.loadFromMemory(mem);
		types.push_back(type);
	}
}

ODF::InvalidCondition::InvalidCondition(const std::string& message) : runtime_error(message) {}
