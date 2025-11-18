#pragma once
#include "Definitions.h"
#include "MemoryDataStream.h"

#include <string>
#include <fstream>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <map>
#include <variant>
#include <iostream>
#include <algorithm>
#include <functional>

class DF_API ODF // optimized object format
{
public:
	// Specifiers
	struct Type;

	struct Status
	{
		enum Value {
			Ok = 0,
			FileOpenError,
			RWError,
			FileCloseError,
			ParseError
		} value;

		operator bool();
		bool operator!();
		bool operator!=(Status other);
		bool operator==(Status other);
		Status operator=(Status other);
		Status(Value val = Status::Ok);
	};

	class TypeSpecifier // THe size specifier specifier is removed automatically when impicitely used.
	{
	public:
		typedef unsigned char Type;
		Type type;
		inline Type byte() const; // returns just the raw byte.
		inline Type sizeSpec() const;
		inline bool isSigned() const;
		inline bool isUnsigned() const;

		operator Type() const;
		TypeSpecifier& operator=(unsigned char uc);
		friend TypeSpecifier operator<<(TypeSpecifier spec, unsigned char shiftBy);
		friend TypeSpecifier operator>>(TypeSpecifier spec, unsigned char shiftBy);
		friend TypeSpecifier operator&(TypeSpecifier spec, unsigned char byte);
		friend TypeSpecifier operator|(TypeSpecifier spec, unsigned char byte);

		TypeSpecifier(Type t = 0);

		static constexpr Type NULLTYPE = 0;
		static constexpr Type UNDEFINED = 1;
		static constexpr Type BYTE = 2;
		static constexpr Type UBYTE = 3;
		static constexpr Type SHORT = 4;
		static constexpr Type USHORT = 5;
		static constexpr Type INT = 6;
		static constexpr Type UINT = 7;
		static constexpr Type LONG = 8;
		static constexpr Type ULONG = 9;
		static constexpr Type FLOAT = 10;
		static constexpr Type DOUBLE = 11;
		static constexpr Type CSTR = 12;
		static constexpr Type WSTR = 13;
	};

	struct SizeSpecifier
	{
		static constexpr size_t OverflowSize8bit = 256;
		static constexpr size_t OverflowSize16bit = 65'536;
		static constexpr size_t OverflowSize32bit = 4'294'967'296;
		// no OverflowSize64bit, as it would be zero on a 64bit system
		size_t actualSize;

		TypeSpecifier sizeSpecifierSpecifier() const; // wow. Use like TypeSpecifier =	
		void load(MemoryDataStream& stream, TypeSpecifier type);
		void save(MemoryDataStream& stream, TypeSpecifier type) const; // explicit size. Only the last 2 bits matter.
		void save(MemoryDataStream& stream) const; // implicit size, size specifier specifier returned by sizeSpecifierSpecifier
	};

	struct MixedObjectSpecifier
	{
		typedef std::pair<std::string, Type> IteratorType;
		std::map<std::string, Type*> properties; // use an ordered map to ensure iteration order is always the same.
		void saveToMemory(MemoryDataStream& mem) const;
	};

	struct FixedObjectSpecifier
	{
		Type* header;
		std::vector<std::string> keys;
		void saveToMemory(MemoryDataStream& mem) const;
	};

	typedef std::variant<MixedObjectSpecifier, FixedObjectSpecifier> ObjectSpecifier;
	struct Type // a full type header
	{ // pointers are used as optionals. Deallocated in destructor
		TypeSpecifier type;
		ObjectSpecifier* obj;
		SizeSpecifier* size;

		void clear(); // deletes every optional specifier
		Status saveToMemory(MemoryDataStream& mem) const;
		void setType(TypeSpecifier type); // use this, otherwise the object might throw exceptions or undefined behaiviour on save
		inline bool isPrimitive() const; // True if no specifies other than TypeSpecifier are needed
		inline bool needsObjectSpecifier() const;
		inline bool needsSizeSpecifier() const;
		inline bool isFixed() const;
		inline bool isMixed() const;
		bool isString() const;

		Type& operator=(const Type& other);

		Type();
		Type(const Type& other);
		~Type();
	};

	// Members
	Type type;

	// Interface

	// load and save functions
	Status saveToMemory(MemoryDataStream& mem) const;
	Status saveToMemory(char* destination, size_t size) const;
	Status loadFromMemory(MemoryDataStream& mem);
	Status loadFromMemory(const char* data, size_t size);

	Status saveToFile(std::string file);
	Status loadFromFile(std::string file);

	Status saveToStream(std::ostream& out, bool binary = true);
	Status loadFromStream(std::istream& in, bool binary = true);

private:
	Status saveBody(MemoryDataStream& mem) const;

	Status loadHeader(MemoryDataStream& mem);
	Status loadBody(MemoryDataStream& mem);
};

#include "ODF.tpp"