#include "ODF.h"

#pragma region specifiers
ODF::Status ODF::Type::saveToMemory(MemoryDataStream& mem) const
{
	// save primitive type
	mem.write<unsigned char>(type.byte());

	// save specifiers
	if (needsSizeSpecifier())
		size->save(mem);

	if (needsObjectSpecifier())
	{
		// check ObjecSpecifier Type and save it
		if (auto ptr = std::get_if<MixedObjectSpecifier>(obj))
			ptr->saveToMemory(mem);
		else if (auto ptr = std::get_if<FixedObjectSpecifier>(obj))
			ptr->saveToMemory(mem);
	}

	return Status::Ok;
}

ODF::Status ODF::Type::loadFromMemory(MemoryDataStream& mem)
{
	// TODO
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

ODF::TypeSpecifier& ODF::TypeSpecifier::operator=(unsigned char uc)
{
	// TODO: hier return-Anweisung eingeben
	type = uc;
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

bool operator==(const ODF& odf1, const ODF& odf2)
{
	return false;
}

bool ODF::Type::isString() const
{
	return false;
}

inline unsigned char ODF::Type::getswitch() const
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
	default: throw VariantType(0);
	}
}

ODF::Type ODF::getComplexType() const
{
	return Type();
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

ODF::Type::operator unsigned char()
{
	return type;
}

ODF::Type& ODF::Type::operator=(const Type& other)
{
	// TODO: hier return-Anweisung eingeben
	return *this;
}

ODF::Type::Type()
{
	obj = nullptr;
	size = nullptr;
}

ODF::Type::Type(const Type& other)
{
	*this = other;
}

ODF::Type::~Type()
{
}

#pragma endregion
#pragma region ObjectSpecifier
void ODF::MixedObjectSpecifier::saveToMemory(MemoryDataStream& mem) const
{
	// see documentation "Mixed Object Specifier"
	for (const auto& pair_it : properties)
	{
		// save key
		mem.writeStr(pair_it.first);
		// save type
		pair_it.second->saveToMemory(mem);
	}
	// save zero to end object specifier
	mem.write<UINT_8>(0);
}

void ODF::FixedObjectSpecifier::saveToMemory(MemoryDataStream& mem) const
{
	// save type
	header->saveToMemory(mem);
	// save keys
	for (const std::string& key : keys)
		mem.writeStr(key);
	// save zero to end specifier
	mem.write<UINT_8>(0);
}
#pragma endregion
#pragma region SizeSpecifier
ODF::TypeSpecifier ODF::SizeSpecifier::sizeSpecifierSpecifier() const
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

void ODF::SizeSpecifier::load(MemoryDataStream& stream, TypeSpecifier type)
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

void ODF::SizeSpecifier::save(MemoryDataStream& stream, TypeSpecifier type) const
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

void ODF::SizeSpecifier::save(MemoryDataStream& stream) const
{
	save(stream, sizeSpecifierSpecifier());
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
}

inline bool ODF::Type::isPrimitive() const
{
	// TODO
	return false;
}

inline bool ODF::Type::needsObjectSpecifier() const
{
	return false;
}

inline bool ODF::Type::needsSizeSpecifier() const
{
	return false;
}

inline bool ODF::Type::isFixed() const
{
	return false;
}

inline bool ODF::Type::isMixed() const
{
	return false;
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

void ODF::Array::AbstractArray::push_back(const ODF& odf)
{
	list.push_back(odf);
}

void ODF::Array::AbstractArray::push_front(const ODF& odf)
{
	list.insert(list.begin(), odf);
}

void ODF::Array::AbstractArray::erase(size_t index)
{
	list.erase(list.begin() + index);
}

void ODF::Array::AbstractArray::erase(size_t index, size_t numberOfElements)
{
	list.erase(list.begin() + index, list.begin() + index + numberOfElements - 1);
}

void ODF::Array::AbstractArray::erase_range(size_t from, size_t to)
{
	list.erase(list.begin() + from, list.begin() + to);
}

void ODF::Array::AbstractArray::insert(size_t where, const ODF& odf)
{
	list.insert(list.begin() + where, odf);
}

size_t ODF::Array::AbstractArray::find(const ODF& odf)
{
	// do a linear search
	for (size_t i = 0; i < list.size(); i++)
		if (list[i] == odf)
			return i;
	return -1; // corresponds to UINT64_MAX
}

size_t ODF::Array::AbstractArray::size() const
{
	return list.size();
}

ODF& ODF::Array::AbstractArray::operator[](size_t index)
{
	return list[index];
}

const ODF& ODF::Array::AbstractArray::operator[](size_t index) const
{
	return list[index];
}

void ODF::Array::FixedArray::push_back(const ODF& odf)
{
	// check type, then push_back, otherwise register fail
	if (odf.getComplexType() == forcedType)
		list.push_back(odf);
	else if (fails)
		fails->push_back(&odf);
}

void ODF::Array::FixedArray::push_front(const ODF& odf)
{
	// check type, then push_back, otherwise register fail
	if (odf.getComplexType() == forcedType)
		list.insert(list.begin(), odf);
	else if (fails)
		fails->push_back(&odf);
}

void ODF::Array::FixedArray::insert(size_t where, const ODF& odf)
{
	// check type, then push_back, otherwise register fail
	if (odf.getComplexType() == forcedType)
		list.insert(list.begin() + where, odf);
	else if (fails)
		fails->push_back(&odf);
}

void ODF::Array::FixedArray::setType(const Type& type)
{
}

inline const ODF::Type& ODF::Array::FixedArray::getType()
{
	// TODO: hier return-Anweisung eingeben
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

ODF::Array::FixedArray::InvalidTypeChange::InvalidTypeChange(std::string message) : runtime_error(message)
{
}

void ODF::Object::makeMixed()
{
}

bool ODF::Object::canBeFixed()
{
	return false;
}

bool ODF::Object::tryFixing()
{
	return false;
}

bool ODF::Object::isMixed()
{
	return false;
}

ODF::Type ODF::Object::getFixedDatatype()
{
	return Type();
}

ODF::Type ODF::Object::getTargetDatatype()
{
	return Type();
}

void ODF::Array::makeMixed()
{
}

bool ODF::Array::canBeFixed()
{
	return false;
}

bool ODF::Array::tryFixing()
{
	return false;
}

bool ODF::Array::isMixed()
{
	return false;
}

ODF::Type ODF::Array::getFixedDatatype()
{
	return Type();
}

ODF::Type ODF::Array::getTargetDatatype()
{
	return Type();
}
