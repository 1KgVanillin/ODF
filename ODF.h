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
#include <optional>


class DF_API ODF // optimized object format
{
public:
	// error types
	struct Config
	{
		static bool PreventAutomaticMergeLossString;
		static bool PreventAutomaticMergeLossFloat;
		static unsigned char DefaultIntegerMergeSize;
	};
	
	// generic development exception
	struct SixSeven : public std::runtime_error
	{
		SixSeven(const std::string& message = "67 (six - seven)");
	};
	// Thrown if a member function that is fixed on a specific type is called for an invalid type
	// For example if push_back would be called on a non list element
	struct TypeMismatch : public std::runtime_error
	{
		TypeMismatch(const std::string& message = "TypeMismatch exception");
	};
	// Throw if any condition is reached that should be unreachable in a correct implementation. Should in theory never be thrown
	struct InvalidCondition : public std::runtime_error
	{
		InvalidCondition(const std::string& message = "InvalidCondition triggered");
	};
	// Throw if a specifier contains a wrong number of elements or properties for loading a type
	struct SpecifierMismatch : public std::runtime_error
	{
		SpecifierMismatch(const std::string& message = "SpecifierMismatch exception");
	};
	// Thrown if a function is called that is not valid for the current type, (for example calling push_back on an integer or setFixType on a mixed List)
	struct InvalidType : public std::runtime_error
	{
		InvalidType(const std::string& message = "InvalidType exception");
	};
	struct InvalidConversion : public std::runtime_error
	{
		InvalidConversion(const std::string& message = "InvalidConversion exception");
	};


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
		bool operator!=(Value other);
		bool operator==(Status other);
		bool operator==(Value other);
		Status operator=(Status other);
		Status(Value val = Status::Ok);
	};

	class AbstractDataObject
	{
	public:
		virtual void saveToMemory(MemoryDataStream& mem) const = 0;
		virtual void loadFromMemory(MemoryDataStream& mem) = 0;
	};

	class TypeSpecifier : public AbstractDataObject // The size specifier specifier is removed automatically when impicitely used.
	{
	public:
		typedef unsigned char Type;
		Type type;
		inline unsigned char byte() const; // returns just the raw byte.
		inline Type sizeSpec() const;
		inline bool isSigned() const;
		inline bool isUnsigned() const;
		inline bool isMixed() const;
		inline bool isFixed() const;
		inline bool isObject() const;
		inline bool isList() const;
		inline Type smallType() const;
		inline Type withoutSSS() const; // without size specifier specifier

		void saveToMemory(MemoryDataStream& mem) const override;
		void loadFromMemory(MemoryDataStream& mem) override;

		operator Type() const;
		TypeSpecifier& operator=(unsigned char byte);
		friend TypeSpecifier operator<<(TypeSpecifier spec, unsigned char shiftBy);
		friend TypeSpecifier operator>>(TypeSpecifier spec, unsigned char shiftBy);
		friend TypeSpecifier operator&(TypeSpecifier spec, unsigned char byte);
		friend TypeSpecifier operator|(TypeSpecifier spec, unsigned char byte);
		friend bool operator==(TypeSpecifier spec1, TypeSpecifier spec2);
		friend bool operator!=(TypeSpecifier spec1, TypeSpecifier spec2);
		friend bool operator==(TypeSpecifier spec, TypeSpecifier::Type type);
		friend bool operator!=(TypeSpecifier spec, TypeSpecifier::Type type);

		TypeSpecifier(Type t = 0);

		// TODO change values to new notation (see v2.md)
		static constexpr Type FLAG_MIXED = 0b0010'0000;
		static constexpr Type FLAG_UNSIGNED = 0b0010'0000;
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
		static constexpr Type NULLTYPE = FLAG_NULL;
		static constexpr Type UNDEFINED = FLAG_NULL | FLAG_MIXED;
		static constexpr Type BYTE = FLAG_BYTE;
		static constexpr Type UBYTE = FLAG_BYTE | FLAG_UNSIGNED;
		static constexpr Type SHORT = FLAG_SHORT;
		static constexpr Type USHORT = FLAG_SHORT | FLAG_UNSIGNED;
		static constexpr Type INT = FLAG_INT;
		static constexpr Type UINT = FLAG_INT | FLAG_UNSIGNED;
		static constexpr Type LONG = FLAG_LONG;
		static constexpr Type ULONG = FLAG_LONG | FLAG_UNSIGNED;
		static constexpr Type FLOAT = FLAG_FLOATING_POINT_TYPE | FLAG_LOW_PREC;
		static constexpr Type DOUBLE = FLAG_FLOATING_POINT_TYPE | FLAG_HIGH_PREC;
		static constexpr Type CSTR = FLAG_STRING | FLAG_LOW_PREC;
		static constexpr Type WSTR = FLAG_STRING | FLAG_HIGH_PREC;
		static constexpr Type FXOBJ = FLAG_OBJ;
		static constexpr Type MXOBJ = FLAG_OBJ | FLAG_MIXED;
		static constexpr Type FXLIST = FLAG_LIST;
		static constexpr Type MXLIST = FLAG_LIST | FLAG_MIXED;
	};

	struct SizeSpecifier : public AbstractDataObject
	{
		static constexpr size_t OverflowSize8bit = 256;
		static constexpr size_t OverflowSize16bit = 65'536;
		static constexpr size_t OverflowSize32bit = 4'294'967'296;
		// no OverflowSize64bit, as it would be zero on a 64bit system
		size_t actualSize;

		size_t operator=(size_t size);

		TypeSpecifier sizeSpecifierSpecifier(TypeSpecifier original = 0) const; // wow. Use like type = spec.sizeSpecSpec(type). Selects the sizeSpecifierSpecifier as the smallest value possible that still fits
		void load(MemoryDataStream& stream, TypeSpecifier type);
		void saveToMemory(MemoryDataStream& stream, TypeSpecifier type) const; // explicit size. Only the last 2 bits matter.
		void saveToMemory(MemoryDataStream& stream) const override; // implicit size, size specifier specifier returned by sizeSpecifierSpecifier
		void loadFromMemory(MemoryDataStream& mem) override; // must not be used, will throw immediatly. This is because a sizeSpecifierSpecifier is needed to load a sizeSpecifier properlys. Maybe make this safer in the future TODO
		SizeSpecifier() = default;
	};

	enum class TypeClass
	{
		Int,
		Float,
		String,
		List,
		Object
	};

	// forward declarations
	struct FixedArraySpecifier;
	struct MixedArraySpecifier;
	struct FixedObjectSpecifier;
	struct MixedObjectSpecifier;

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
	typedef std::variant<MixedArraySpecifier, FixedArraySpecifier> ArraySpecifier;
	struct Type // a full type header
	{ // pointers are used as optionals. Deallocated in destructor
		TypeSpecifier type;
		typedef std::variant<ObjectSpecifier, ArraySpecifier> ComplexSpecifier;
		ComplexSpecifier* complexSpec; // holds object specification optionallly

		void clear(); // deletes every optional specifier
		Status saveToMemory(MemoryDataStream& mem) const;
		Status loadFromMemory(MemoryDataStream& mem);
		void setType(TypeSpecifier type); // use this, otherwise the object might throw exceptions or undefined behaiviour on save // TODO
		inline bool isPrimitive() const; // True if no specifies other than TypeSpecifier are needed // TODO
		inline bool needsObjectSpecifier() const;
		inline bool needsSizeSpecifier() const;
		inline bool isFixed() const;
		inline bool isMixed() const;
		inline bool isObject() const;
		inline bool isList() const;
		bool isString() const; // TODO
		inline TypeSpecifier getswitch() const; // gets the switch() case compatible Type (without SizeSpecifier)
		inline VariantType getVariantType() const; // TODO
		SizeSpecifier* sizeSpecifier() const; // returns nullptr if there is no sizespecifier
		ArraySpecifier* arraySpecifier() const;
		FixedArraySpecifier* fixedArraySpecifier() const;
		MixedArraySpecifier* mixedArraySpecifier() const;
		ObjectSpecifier* objectSpecifier() const;
		FixedObjectSpecifier* fixedObjectSpecifier() const;
		MixedObjectSpecifier* mixedObjectSpecifier() const;

		void setFixType(const Type& type);
		const Type* getFixType() const; // returns the fixtype if the current type is complex and fixed. otherwise returns nullptr
		TypeClass getTypeClass() const;
		const std::vector<Type>* getArrayTypes() const; // returns the types vector of the mixed array specifier if mixed, otherwise nullptr
		const std::map<std::string, Type>* getObjectTypes() const; // returns the types vector of the mixed object specifier if mixed, otherwise nullptr
		void setSSS(unsigned char sss);
		void destroyComplexSpec();
		void makeComplexSpec(); // memory leak safe. Makes sure complexSpec is valid, keeps the old one if one is already existing
		void resetComplexSpec(); // memory leak safe. If an old complexSpec exists it gets destroyed, then a fresh one is allcated. Maybe fix performace of this by keeping and overwriting the old object? TODO

		friend bool operator==(const Type& t1, const Type& t2);
		friend bool operator!=(const Type& t1, const Type& t2);

		operator unsigned char();
		Type& operator=(const Type& other);
		TypeSpecifier operator=(TypeSpecifier type);

		Type();
		Type(const Type& other);
		Type(TypeSpecifier type);
		~Type();
	};

	// The ArraySpecifier holds all type information that is needed to load or save an array.
	// saves or loads the size specifier on save
	struct FixedArraySpecifier : public AbstractDataObject
	{
		SizeSpecifier size;
		Type fixType; // always holds a value, is just a pointer because of the forward declaring
		void saveToMemory(MemoryDataStream& mem) const override;
		void loadFromMemory(MemoryDataStream& mem) override;
	};

	struct MixedArraySpecifier : public AbstractDataObject
	{
		std::vector<Type> types;

		void saveToMemory(MemoryDataStream& mem) const override;
		void loadFromMemory(MemoryDataStream& mem) override;
	};

	struct FixedObjectSpecifier : public AbstractDataObject
	{
		Type fixType;
		std::vector<std::string> keys;
		void saveToMemory(MemoryDataStream& mem) const override;
		void loadFromMemory(MemoryDataStream& mem) override;
	};

	struct MixedObjectSpecifier : public AbstractDataObject
	{
		typedef const std::pair<std::string, Type>& ObjectIterator;
		std::map<std::string, Type> properties; // use an ordered map to ensure iteration order is always the same.
		void saveToMemory(MemoryDataStream& mem) const override;
		void loadFromMemory(MemoryDataStream& mem) override;
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

	class AbstractObject : public AbstractDataObject
	{
		protected:
		std::map<std::string, ODF> map;
	public:
		typedef std::pair<std::string, ODF> Pair;
		typedef std::map<std::string, ODF>::const_iterator ConstIterator;
		virtual void insert(std::string& key, const ODF& odf);
		virtual void insert(const Pair& value);
		virtual void insert(const std::map<std::string, ODF>& map);
		virtual void insert(const std::unordered_map<std::string, ODF>& umap);
		virtual void insert(const std::vector<Pair>& pairs);
		virtual void insert(const std::initializer_list<Pair>& pairs);
		virtual void erase(const std::string& key);
		virtual bool contains(const std::string& key) const;
		virtual size_t count(const std::string& key) const;
		virtual size_t size() const;

		virtual ODF& at(const std::string& key);
		virtual const ODF& at(const std::string& key) const;
		virtual ODF& operator[](const std::string& key);

		virtual ConstIterator begin() const noexcept;
		virtual ConstIterator end() const noexcept;
		virtual std::map<std::string, ODF>& getObjectContainer();
	};

	class Object : public AbstractType, public AbstractObject
	{
	public:
		class FixedObject : public AbstractObject
		{
		public:
			void saveToMemory(MemoryDataStream& mem) const override;
			void loadFromMemory(MemoryDataStream& mem) override;

			FixedObject();
		};

		class MixedObject : public AbstractObject
		{
		public:
			void saveToMemory(MemoryDataStream& mem) const override;
			void loadFromMemory(MemoryDataStream& mem) override;

			MixedObject();
			MixedObject(const std::initializer_list<Pair>& elements);
		};
		
		std::variant<FixedObject, MixedObject> object;

		void makeMixed() const override; // TODO
		bool canBeFixed() const override; // TODO
		bool tryFixing() const override; // TODO
		bool isMixed() const override; // TODO
		Type getFixedDatatype() const override; // TODO
		Type getTargetDatatype() const override; // TODO
	};


	// The AbstractArray is an implementation of the mixed array.
	// The mixedArray inherits everything from it.
	// The FixedArray inherits erase, etc. and overwrites the push and insert functions to make a fixed check
	class AbstractArray : public AbstractDataObject
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
		virtual void clear();
		virtual void resize(size_t newSize);

		virtual ODF& operator[](size_t index);
		virtual const ODF& operator[](size_t index) const;

		virtual ODF* begin() const noexcept;
		virtual ODF* end() const noexcept;
		virtual std::vector<ODF>& getArrayContainer();
	};
	
	class List : public AbstractType, public AbstractArray
	{
	public:
		struct PreventSimilarTypeMerge { PreventSimilarTypeMerge() = default; };
		class FixedArray : public AbstractArray
		{
			Type fixType; // uses a complex type to compare. Example: A list made out of FixedArrays holding different types would have the same Variant- and Primitive- type but different types in the final document
			std::vector<const ODF*>* fails; // if enabled, this is not nullptr. It stores a const ptr to every ODF object that wasn't inserted into the list because of a type mismatch
		public:
			void push_back(const ODF& odf) override;
			void push_front(const ODF& odf) override;
			void insert(size_t where, const ODF& odf) override;
			void updateSize(Type& size) const;

			template<typename T>
			void convert(std::function<T(const ODF& odf)> converter); // TODO

			void setType(const Type& type); // can only be called on an empty list, otherwise throws exception // TODO
			void resize(size_t newSize) override;
			const Type& getType() const; // TODO

			void enableFailRegistry(bool enable = true); // TODO
			bool fail() const;
			const std::vector<const ODF*>& getFails(); // TODO

			void saveToMemory(MemoryDataStream& mem) const override;
			void loadFromMemory(MemoryDataStream& mem) override;

			struct InvalidTypeChange : public std::runtime_error
			{
				InvalidTypeChange(std::string message = "Error: ODF::Array::FixedArray::setType called on non empty array");
			};

			FixedArray();
			template<typename T>
			FixedArray(const std::initializer_list<T>& initializer_list) : fails(nullptr)
			{
				for (const T& t : initializer_list)
					push_back(t);
			}
			template<typename T>
			FixedArray(const std::vector<T>& vector) : fails(nullptr)
			{
				for (const T& t : vector)
					push_back(t);
			}
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

			void saveToMemory(MemoryDataStream& mem) const override;
			void loadFromMemory(MemoryDataStream& mem) override;

			MixedArray();
			MixedArray(const std::initializer_list<ODF>& initializer_list);
		};

		std::variant<FixedArray, MixedArray> list;

		// type functions
		void clear();
		void clearAndFix(); // clear and make fixed
		void clearAndFix(const Type& elementType); // clear and make fixed
		void clearAndMix(); // clear and make mixed
		void makeMixed() const override; // TODO
		bool canBeFixed() const override; // TODO
		bool tryFixing() const override; // TODO
		bool isMixed() const override; // TODO
		Type getFixedDatatype() const override; // called on fixed list, returns the actual datatype that every element has. Throws std::bad_variant_access if called on mixed
		Type getTargetDatatype() const override; // TODO, called on mixed list, returns the datatype that every element would have if converted to fixed. THrows std::bad_variant_access if called on fixed

		// List functions
		void push_back(const ODF& odf) override;
		void push_front(const ODF& odf) override;
		void erase(size_t index) override;
		void erase(size_t index, size_t numberOfElements) override;
		void erase_range(size_t from, size_t to) override;
		void insert(size_t where, const ODF& odf) override;
		size_t find(const ODF& odf) override;
		size_t size() const override;
		ODF& operator[](size_t index) override;
		const ODF& operator[](size_t index) const override;
		std::vector<ODF>& getArrayContainer() override;

		void insert(std::string key, ODF& odf) override;

		FixedArray& fixed() const; // just forwards to get<FixedArray>()
		MixedArray& mixed() const; // just forwards to get<MixedArray>()

		void resetAndSetSize(size_t newSize, const std::vector<Type>& newTypes); // this function is called by the root object which loads the specifier to tell the array their size. All current data is lost.
		void resetAndSetSize(size_t newSize, const Type& newType); // this function is called by the root object which loads the specifier to tell the array their size. All current data is lost. Works only on fixed arrays
		void saveToMemory(MemoryDataStream& mem) const override;
		void loadFromMemory(MemoryDataStream& mem) override;

		friend bool operator==(const List& arr1, const List& arr2);
		friend bool operator!=(const List& arr1, const List& arr2);
		List& operator=(const List& other);
		List& operator=(const MixedArray& other);
		List& operator=(const std::initializer_list<ODF>& initializer_list);
		List& operator=(const FixedArray& other);
		template<typename T> List& operator=(const std::initializer_list<T>& initializer_list) { *this = FixedArray(initializer_list); }

		List();
		List(const List& arr);
		List(const MixedArray& marr);
		List(const std::initializer_list<ODF>& initializer_list);
		List(const FixedArray& farr);
		template<typename T> List(const std::initializer_list<T>& initializer_list) : List(FixedArray(initializer_list)) {}
		template<typename T> List(const std::vector<T>& vector) : List(FixedArray(vector)) {}
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
		List // 13
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

	template<typename T>
	static VariantType getVariantTypeFromTemplate() {}; // TODO

	// Interface
	// access operators
	ODF& operator[](size_t index);

	// operator=
	ODF& operator=(const ODF& other); // copy
	// primitive types
	ODF& operator=(INT_8 val);
	ODF& operator=(UINT_8 val);
	ODF& operator=(INT_16 val);
	ODF& operator=(UINT_16 val);
	ODF& operator=(INT_32 val);
	ODF& operator=(UINT_32 val);
	ODF& operator=(INT_64 val);
	ODF& operator=(UINT_64 val);
	ODF& operator=(float val);
	ODF& operator=(double val);
	ODF& operator=(const std::string& str);
	ODF& operator=(const std::wstring& wstr);
	// array types
	ODF& operator=(const List& arr);
	ODF& operator=(const List::MixedArray& marr);
	ODF& operator=(const std::initializer_list<ODF>& initializer_list);
	ODF& operator=(const List::FixedArray& farr);
	template<typename T> ODF& operator=(const std::initializer_list<T>& initializer_list) { *this = List::FixedArray(initializer_list); }
	// object types
	ODF& operator=(const Object& obj);
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
	operator List();
	ODF();
	ODF(const ODF& other);
	ODF(INT_8 val);
	ODF(UINT_8 val);
	ODF(INT_16 val);
	ODF(UINT_16 val);
	ODF(INT_32 val);
	ODF(UINT_32 val);
	ODF(INT_64 val);
	ODF(UINT_64 val);
	ODF(float val);
	ODF(double val);
	ODF(const std::string& str);
	ODF(const char* cstr);
	ODF(const std::wstring& wstr);
	ODF(const wchar_t* wstr);
	ODF(const Object& obj);
	ODF(const List& arr);
	ODF(const List::MixedArray& marr);
	ODF(const std::initializer_list<ODF>& initializer_list);
	ODF(const List::FixedArray& farr);
	template<typename T> ODF(const std::initializer_list<T>& initializer_list) : ODF(List::FixedArray(initializer_list)) {} // although a (ODF(Array(...)) would be sufficient, the fixed array is needed to explicitely invoke the ODF(FixedArray) constructor which explicitely calls operator=(const FixedArray&) which updates the size specifier
	template<typename T> ODF(const std::vector<T>& vector) : ODF(List::FixedArray(vector)) {}

	// ostream printing
	// print flags
	bool printType;
	// print operators
	friend std::ostream& operator<<(std::ostream& out, const ODF& odf);
	friend std::ostream& operator<<(std::ostream& out, const Object& obj);
	friend std::ostream& operator<<(std::ostream& out, const List& list);
	friend std::ostream& operator<<(std::ostream& out, const std::wstring& wstr);
	friend std::ostream& operator<<(std::ostream& out, const Type& type);

	// generic functions
	bool isConvertableTo(const Type& other) const;
	void convertTo(const Type& newType);
	bool covertToIfConvertable(const Type& newType);
	ODF getAs(const Type& newType) const;
	std::optional<ODF> getAsIfConvertable(const Type& newType) const;
	template<typename T>
	T& get()
	{
		return std::get<T>(content);
	}
	template<typename T>
	const T& get() const
	{
		return std::get<T>(content);
	}

	// list functions
	void makeList(); // clears content and converts element to a list
	void makeList(const Type& elementType); // clears content and converts to fxlist of with elementtype
	void makeList(const TypeSpecifier& elementType); // clears content and converts to fxlist of with elementtype
	void push_back(const ODF& odf);
	void push_front(const ODF& odf);
	void erase(size_t index);
	void erase(size_t index, size_t numberOfElements);
	void erase_range(size_t from, size_t to);
	void insert(size_t where, const ODF& odf);
	size_t find(const ODF& odf);
	bool push_back_nothrow(const ODF& odf) noexcept;
	bool push_front_nothrow(const ODF& odf) noexcept;
	bool erase_nothrow(size_t index) noexcept;
	bool erase_nothrow(size_t index, size_t numberOfElements) noexcept;
	bool erase_range_nothrow(size_t from, size_t to) noexcept;
	bool insert_nothrow(size_t where, const ODF& odf) noexcept;
	size_t find_nothrow(const ODF& odf) noexcept;
	size_t size() const;
	void clear();

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
	void updateSize(); // will dereferece a nullptr if the type doesn't has a sizeSpecfier or throw an std::bad_variant_access

	Status saveBody(MemoryDataStream& mem) const;
	Status saveObject(MemoryDataStream& mem) const;
	Status saveArray(MemoryDataStream& mem) const;

	Status loadBody(MemoryDataStream& mem);
	Status loadObject(MemoryDataStream& mem);
	Status loadArray(MemoryDataStream& mem);
};


#include "ODF.tpp"

template<typename T>
inline void ODF::List::FixedArray::convert(std::function<T(const ODF& odf)> converter)
{
}
