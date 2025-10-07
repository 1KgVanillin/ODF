#include "ODF.h"

#pragma region specifiers
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

std::string ODF::PlainTextConverter::toPlainText(MemoryDataStream& mem)
{
	return std::string();
}

bool ODF::PlainTextConverter::toBinary(const std::string& str, MemoryDataStream& dest)
{
	return false;
}

ODF::Status ODF::saveToMemory(MemoryDataStream& mem) const
{

}

ODF::Status ODF::saveToMemory(char* destination, size_t size) const
{
	return Status();
}

ODF::Status ODF::loadFromMemory(MemoryDataStream& mem)
{

}

ODF::Status ODF::loadFromMemory(const char* data, size_t size)
{

}

ODF::Status ODF::saveToFile(std::string file)
{

}

ODF::Status ODF::loadFromFile(std::string file)
{

}

ODF::Status ODF::saveToStream(std::ostream& out, bool binary)
{

}

ODF::Status ODF::loadFromStream(std::istream& in, bool binary)
{

}

