#pragma once
#include "Definitions.h"
#include "MemoryDataStream.h"
#include "GenericDebug.h"

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


/*	Inheritance structure:
*	
*	
* 
* 
*/


class DF_API ODF // optimized object format
{
public:
	// random constants
	static constexpr unsigned char PRECISION_FLAG_CONVERSION_NEEDED = 0b10000000;

	// Specifiers
	struct Type;

	struct Status
	{
		enum Value {
			Ok = 0,
			TypeMismatch,
			FileOpenError,
			RWError,
			FileCloseError,
			ParseError
		} content;

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
		inline bool isMixed() const;
		inline bool isFixed() const;
		inline bool isObject() const;
		inline bool isList() const;

		operator Type() const;
		TypeSpecifier& operator=(unsigned char byte);
		friend TypeSpecifier operator<<(TypeSpecifier spec, unsigned char shiftBy);
		friend TypeSpecifier operator>>(TypeSpecifier spec, unsigned char shiftBy);
		friend TypeSpecifier operator&(TypeSpecifier spec, unsigned char byte);
		friend TypeSpecifier operator|(TypeSpecifier spec, unsigned char byte);

		TypeSpecifier(Type t = 0);

		// TODO change values to new notation (see v2.md)
		static constexpr Type FLAG_FIXED = 0;
		static constexpr Type FLAG_MIXED = 0b0010'0000;
		static constexpr Type FLAG_UNSIGNED = 0b0010'0000;
		static constexpr Type FLAG_SIGNED = 0;
		static constexpr Type FLAG_HIGH_PREC = 0b0010'0000;
		static constexpr Type FLAG_LOW_PREC = 0;
		static constexpr Type FLAG_NULL = 0;
		static constexpr Type FLAG_BYTE = 1;
		static constexpr Type FLAG_SHORT = 2;
		static constexpr Type FLAG_INT = 3;
		static constexpr Type FLAG_LONG = 4;
		static constexpr Type FLAG_FLOATING_POINT_TYPE = 5;
		static constexpr Type FLAG_STRING = 6;
		static constexpr Type FLAG_OBJ = 7;
		static constexpr Type FLAG_LIST = 8;
		static constexpr Type NULLTYPE = FLAG_NULL | FLAG_FIXED;
		static constexpr Type UNDEFINED = FLAG_NULL | FLAG_MIXED;
		static constexpr Type BYTE = FLAG_BYTE | FLAG_SIGNED;
		static constexpr Type UBYTE = FLAG_BYTE | FLAG_UNSIGNED;
		static constexpr Type SHORT = FLAG_SHORT | FLAG_SIGNED;
		static constexpr Type USHORT = FLAG_SHORT | FLAG_UNSIGNED;
		static constexpr Type INT = FLAG_INT | FLAG_SIGNED;
		static constexpr Type UINT = FLAG_INT | FLAG_UNSIGNED;
		static constexpr Type LONG = FLAG_LONG | FLAG_SIGNED;
		static constexpr Type ULONG = FLAG_LONG | FLAG_UNSIGNED;
		static constexpr Type FLOAT = FLAG_FLOATING_POINT_TYPE | FLAG_LOW_PREC;
		static constexpr Type DOUBLE = FLAG_FLOATING_POINT_TYPE | FLAG_HIGH_PREC;
		static constexpr Type CSTR = FLAG_STRING | FLAG_LOW_PREC;
		static constexpr Type WSTR = FLAG_STRING | FLAG_HIGH_PREC;
		static constexpr Type FXOBJ = FLAG_OBJ | FLAG_FIXED;
		static constexpr Type MXOBJ = FLAG_OBJ | FLAG_MIXED;
		static constexpr Type FXLIST = FLAG_LIST | FLAG_FIXED;
		static constexpr Type MXLIST = FLAG_LIST | FLAG_MIXED;
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

	// variant type definitions
	typedef size_t VariantType;
	static constexpr VariantType VT_INT8 = 0;
	static constexpr VariantType VT_UINT8 = 1;
	static constexpr VariantType VT_INT16 = 2;
	static constexpr VariantType VT_UINT16 = 3;
	static constexpr VariantType VT_INT32 = 4;
	static constexpr VariantType VT_UINT32 = 5;
	static constexpr VariantType VT_INT64 = 6;
	static constexpr VariantType VT_UINT64 = 7;
	static constexpr VariantType VT_FLOAT = 8;
	static constexpr VariantType VT_DOUBLE = 9;
	static constexpr VariantType VT_CSTR = 10;
	static constexpr VariantType VT_WSTR = 11;
	static constexpr VariantType VT_OBJ = 12;
	static constexpr VariantType VT_LIST = 13;
	struct VariantTypeConversionError : std::runtime_error { VariantTypeConversionError(std::string message = "Invalid Conversion from type [NULL] or [undefined] to ODF::VariantType"); };

	typedef std::variant<MixedObjectSpecifier, FixedObjectSpecifier> ObjectSpecifier;
	struct Type // a full type header
	{ // pointers are used as optionals. Deallocated in destructor
		TypeSpecifier type;
		ObjectSpecifier* obj;
		SizeSpecifier* size;

		void clear(); // deletes every optional specifier
		Status saveToMemory(MemoryDataStream& mem) const;
		Status loadFromMemory(MemoryDataStream& mem);
		void setType(TypeSpecifier type); // use this, otherwise the object might throw exceptions or undefined behaiviour on save // TODO
		inline bool isPrimitive() const; // True if no specifies other than TypeSpecifier are needed // TODO
		inline bool needsObjectSpecifier() const;
		inline bool needsSizeSpecifier() const;
		inline bool isFixed() const; // TODO
		inline bool isMixed() const; // TODO
		bool isString() const; // TODO
		inline unsigned char getswitch() const; // gets the switch() case compatible Type (without SizeSpecifier)
		inline VariantType getVariantType() const; // TODO

		operator unsigned char();
		Type& operator=(const Type& other); // TODO
		TypeSpecifier operator=(TypeSpecifier type);

		Type();
		Type(const Type& other);
		~Type();
	};

	enum class TypeClass
	{
		Int,
		Float,
		Other
	};

	// Data Types
	class AbstractType
	{
	public:
		virtual void makeMixed() const = 0;
		virtual bool canBeFixed() const = 0;
		virtual bool tryFixing() const = 0;
		virtual bool isMixed() const = 0;
		virtual Type getFixedDatatype() const = 0;
		virtual Type getTargetDatatype() const = 0;
	};

	class Object : public AbstractType
	{
	public:
		class MixedObject
		{
		public:
		};

		class FixedObject
		{
		public:
		};
		
		std::variant<FixedObject, MixedObject> object;

		void makeMixed() const override; // TODO
		bool canBeFixed() const override; // TODO
		bool tryFixing() const override; // TODO
		bool isMixed() const override; // TODO
		Type getFixedDatatype() const override; // TODO
		Type getTargetDatatype() const override; // TODO
	};
	
	class Array : public AbstractType
	{
	public:
		// The AbstractArray is an implementation of the mixed array.
		// The mixedArray inherits everything from it.
		// The FixedArray inherits erase, etc. and overwrites the push and insert functions to make a fixed check
		class AbstractArray
		{
		protected:
			std::vector<ODF> list;
		public:
			virtual void push_back(const ODF& odf);
			virtual void push_front(const ODF& odf);
			virtual void erase(size_t index);
			virtual void erase(size_t index, size_t numberOfElements);
			virtual void erase_range(size_t from, size_t to);
			virtual void insert(size_t where, const ODF& odf);
			virtual size_t find(const ODF& odf);
			virtual size_t size() const;

			ODF& operator[](size_t index);
			const ODF& operator[](size_t index) const;
		};

		class FixedArray : public AbstractArray
		{
			Type forcedType; // uses a complex type to compare. Example: A list made out of FixedArrays holding different types would have the same Variant- and Primitive- type but different types in the final document
			std::vector<const ODF*>* fails; // if enabled, this is not nullptr. It stores a const ptr to every ODF object that wasn't inserted into the list because of a type mismatch
		public:
			void push_back(const ODF& odf) override;
			void push_front(const ODF& odf) override;
			void insert(size_t where, const ODF& odf) override;

			template<typename T>
			void convert(std::function<T(const ODF& odf)> converter); // TODO

			void setType(const Type& type); // can only be called on an empty list, otherwise throws exception // TODO
			const Type& getType(); // TODO

			void enableFailRegistry(bool enable = true); // TODO
			bool fail() const;
			const std::vector<const ODF*>& getFails(); // TODO

			FixedArray();

			struct InvalidTypeChange : public std::runtime_error
			{
				InvalidTypeChange(std::string message = "Error: ODF::Array::FixedArray::setType called on non empty array");
			};
		};

		// inherits all vector functions from AbstractArray
		class MixedArray : public AbstractArray
		{
		public:
			struct FixInfo
			{
				bool tcmatch;
				Type targetType;
				TypeClass tc;
			};
			bool canBeFixed(FixInfo* info = nullptr) const;
			FixedArray* tryFixing(FixInfo* info = nullptr); // returned structure is on the heap and needs to be deallocated. nullptr on error
		};

		std::variant<FixedArray, MixedArray> list;

		void makeMixed() const override; // TODO
		bool canBeFixed() const override; // TODO
		bool tryFixing() const override; // TODO
		bool isMixed() const override; // TODO
		Type getFixedDatatype() const override; // TODO
		Type getTargetDatatype() const override; // TODO
	};
		

	// Value
	typedef std::variant<
		INT_8, // 0
		UINT_8, // 1
		INT_16, // 2
		UINT_16, // 3
		INT_32, // 4
		UINT_32, // 5
		INT_64, // 6
		UINT_64, // 7
		float, // 8
		double, // 9
		std::string, // 10
		std::wstring, // 11
		Object, // 12
		Array // 13
	> ValueType;
	ValueType content;

	// Members
	Type type;

	// Type Interface
	VariantType getVariantType() const;
	Type getComplexType() const; // TODO
	TypeSpecifier getPrimitiveType() const; // TODO
	TypeClass getTypeClass() const; // TODO
	unsigned char getPrecision() const; // TODO
	static TypeSpecifier getTypeFromTC(TypeClass tc, unsigned char precision); // TODO

	// Conversion Interface
	ODF getAs(TypeSpecifier spec) const; // only for primitive types // TODO

	template<typename T>
	static VariantType getVariantTypeFromTemplate() {}; // TODO

	// Interface
	static bool printType;
	ODF& operator=(const ODF& other);
	ODF& operator=(const INT_8& val);
	ODF& operator=(const UINT_8& val);
	ODF& operator=(const INT_16& val);
	ODF& operator=(const UINT_16& val);
	ODF& operator=(const INT_32& val);
	ODF& operator=(const UINT_32& val);
	ODF& operator=(const INT_64& val);
	ODF& operator=(const UINT_64& val);
	ODF& operator=(const float& val);
	ODF& operator=(const double& val);
	ODF& operator=(const std::string& str);
	ODF& operator=(const std::wstring& wstr);
	ODF& operator=(const Object& obj);
	ODF& operator=(const Array& arr);
	operator INT_8();
	operator UINT_8();
	operator INT_16();
	operator UINT_16();
	operator INT_32();
	operator UINT_32();
	operator INT_64();
	operator UINT_64();
	operator float();
	operator double();
	operator std::string();
	operator std::wstring();
	operator Object();
	operator Array();
	friend std::ostream& operator<<(std::ostream& out, const ODF& odf);
	friend std::ostream& operator<<(std::ostream& out, const Object& obj);
	friend std::ostream& operator<<(std::ostream& out, const Array& list);
	friend std::ostream& operator<<(std::ostream& out, const std::wstring& wstr);

	// Operators
	friend bool operator== (const ODF& odf1, const ODF& odf2); // TODO

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
	Status saveObject(MemoryDataStream& mem) const;
	Status saveArray(MemoryDataStream& mem) const;

	Status loadBody(MemoryDataStream& mem);
	Status loadObject(MemoryDataStream& mem);
	Status loadArray(MemoryDataStream& mem);
};

#include "ODF.tpp"

template<typename T>
inline void ODF::Array::FixedArray::convert(std::function<T(const ODF& odf)> converter)
{
}
