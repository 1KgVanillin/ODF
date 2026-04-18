#pragma once
#include "Definitions.h"
#include "MemoryDataStream.h"
#include "GenericDebug.h"

#include <string>
#include <fstream>
#include <cstdint>
#include <vector>
#include <map>
#include <unordered_map>
#include <variant>
#include <iostream>
#include <algorithm>
#include <functional>
#include <optional>

#define ODF_THREADSAFE
#ifdef ODF_THREADSAFE
#include <mutex>
#define ODF_THREADSAFE_PRINT std::lock_guard<std::mutex> threadsafe_print_guard(ODF::printMutex)
#else
#define ODF_THREADSAFE_PRINT
#endif
#define ODF_DISABLE_TYPE_PRINT bool odf_print_type_backup = ODF::printType; ODF::printType = false;
#define ODF_RESTORE_TYPE_PRINT ODF::printType = odf_print_type_backup


class DF_API ODF // optimized object format
{
public:
	// error types
	struct DF_API Config
	{
		static bool PreventAutomaticMergeLossString;
		static bool PreventAutomaticMergeLossFloat;
		static unsigned char DefaultIntegerMergeSize;
	};
	
	struct DF_API ComplexExplicitor
	{
	static struct OBJSTRUCT { private: OBJSTRUCT() = default; friend class ODF; } __MOBJ;
	static struct FOBJSTRUCT { private: FOBJSTRUCT() = default; friend class ODF; } __FOBJ;
	};

	// generic development exception
	struct DF_API SixSeven : public std::runtime_error
	{
		SixSeven(const std::string& message = "67 (six - seven)");
	};
	// Thrown if a member function that is fixed on a specific type is called for an invalid type
	// For example if push_back would be called on a non list element
	struct DF_API TypeMismatch : public std::runtime_error
	{
		TypeMismatch(const std::string& message = "TypeMismatch exception");
	};
	// Throw if any condition is reached that should be unreachable in a correct implementation. Should in theory never be thrown
	struct DF_API InvalidCondition : public std::runtime_error
	{
		InvalidCondition(const std::string& message = "InvalidCondition triggered");
	};
	// Throw if a specifier contains a wrong number of elements or properties for loading a type
	struct DF_API SpecifierMismatch : public std::runtime_error
	{
		SpecifierMismatch(const std::string& message = "SpecifierMismatch exception");
	};
	// Thrown if a function is called that is not valid for the current type, (for example calling push_back on an integer or setFixType on a mixed List)
	struct DF_API InvalidType : public std::runtime_error
	{
		InvalidType(const std::string& message = "InvalidType exception");
	};
	// Thrown if a conversion function is called for object of a different typeclass
	struct DF_API InvalidConversion : public std::runtime_error
	{
		InvalidConversion(const std::string& message = "InvalidConversion exception");
	};
	// Thrown if a function attempts to change a type that is currently immutable
	struct DF_API InvalidTypeMutation : public std::runtime_error
	{
		InvalidTypeMutation(const std::string& message = "InvalidTypeMutation exception");
	};
	// Thrown if an type imported by "import" is not found
	struct DF_API UnresolvedImport : public std::runtime_error
	{
		UnresolvedImport(const std::string& message = "UnresovledImport exception");
	};
	struct DF_API InvalidParseInformation : public std::runtime_error
	{
		InvalidParseInformation(const std::string& message = "InvalidParseInformation exception");
	};
	struct VTypeDataAlreadyExists : public std::runtime_error
	{
		VTypeDataAlreadyExists(const std::string& message = "VTypeDataAlreadyExists exception");
	};
	// thrown in a typeID is specified as built-in though the runtimeID is specified otherwise.
	struct InvalidTypeID : public std::runtime_error
	{
		InvalidTypeID(const std::string& message = "InvalidTypeID exception");
	};

	struct DF_API PrettyPrintInfo
	{
		unsigned short indent;

		void pre(std::ostream& out);
		void post(std::ostream& out);

		PrettyPrintInfo();
	};

	struct DF_API Status
	{
		enum Value {
			Ok = 0,
			Virtual, // the type that was loaded was virtual and not a USETYPE, so no type is returned. The parsing of the type was succesfull. Check available bytes in Input stream and load type again.
			InvalidLocalTypeID, // the local type wasn't found.
			InvalidTypeImport, // the typeid that was requested to be imported wasn't found
			InvalidType,
			NoPoolProvided, // The type is not a built but no pool to import it from is provided
			TypeMismatch,
			FileOpenError,
			RWError,
			FileCloseError,
			ParseError
		} content;

		operator Value();
		bool operator!();
		bool operator!=(Status other);
		bool operator!=(Value other);
		bool operator==(Status other);
		bool operator==(Value other);
		Status operator=(Status other);
		Status(Value val = Status::Ok);
	};

	struct Type;
	// define a hash function for using Type as key in a map in VTypeInfo
	struct TypeHasher
	{
		size_t operator()(const Type& type) const;
	};

	struct ConstParseInfo;
	struct DF_API VTypeInfo
	{
		struct TypeID
		{
			static constexpr unsigned char WildcardSSS = 0xFF; // In case the sss isn't needed, it is set to this value, to prevent accidental use.
			size_t runtimeID; // the id that the TypeID is translated to at runtime
			unsigned char sss; // size specifier specifier. ReplacesOldID is true if sss == 0
			TypeID() = default;
			TypeID(size_t runtimeID, unsigned char sss);
			void saveToMemory(MemoryDataStream& mem) const;
			void saveToMemory(MemoryDataStream& mem, unsigned char runtimeID) const;
			unsigned char addSSS(unsigned char typeSpec) const;
			static unsigned char smallestSSS(size_t type);
			void matchSSS(); // selects the smallest possible sss that fits the runtimeID
			bool operator== (const TypeID& other) const; // as this function might be used in a hashmap, it will return true if runtimeID and sss are equal, or if runtimeIDs are equal and at least one sss is Wildcard.

			void operator++();
		};
		struct TypeIDHasher // hashes only the runtimeID
		{
			size_t operator()(const TypeID& id) const;
		};
		std::unordered_map<Type, TypeID, TypeHasher> definedTypes; // holds all types that should be defined by the current document and their corresponding type.
		std::unordered_map<size_t, TypeID> imports; // maps globalID (key) to localID (value)
		std::unordered_map<size_t, TypeID> exports; // maps localID (key) to globalID (value)

		void addExport(size_t id, unsigned char sss); // throws VTypeDataAlreadyExists if the element already exists
		void addExport(size_t localID, size_t globalID, unsigned char sss); // throws VTypeDataAlreadyExists if the element (localID) already exists.
		void addImport(size_t id, unsigned char sss); // throws VTypeDataAlreadyExists if the element already exists
		void addImport(size_t globalID, size_t localID, unsigned char sss); // throws VTypeDataAlreadyExists if the element (globalID) already exists
		void addDefinedType(const Type& type, size_t typeID, unsigned char sss); // throws VTypeDataAlreadyExists if the element already exists
		void addDefinedType(const Type& type, TypeID typeID); // throws VTypeDataAlreadyExists if the element already exists

		TypeID getFreeTypeID() const; // returns the smallest possible unused typeID
		static unsigned char getNextBIOTypeID(unsigned char type = 0);

		bool isAlreadyDefined(const Type& type) const;
		bool isAlreadyDefined(TypeID typeID) const;
		std::optional<ODF::VTypeInfo::TypeID> getTypeID(const Type& type) const; // returns nullopt if the type isn't defined
		bool useType(MemoryDataStream& mem, const Type& type, const ConstParseInfo& parseInfo); // Use the type with USETYPE if it is defined (return true), otherwise save it normally (return false).
		void saveDefinedTypes(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const;
		void saveImports(MemoryDataStream& mem) const;
		void saveExports(MemoryDataStream& mem) const;
		void saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const;
	};

	typedef std::unordered_map<VTypeInfo::TypeID, Type, VTypeInfo::TypeIDHasher> PoolType;
	typedef std::shared_ptr<PoolType> Pool;
	class DF_API PoolCollection : public std::enable_shared_from_this<PoolCollection> // see ODF::PoolCollection::parse in ODF.cpp for more details on enable_shared_from_this
	{
	public:
		using TypeID = VTypeInfo::TypeID;

		std::vector<Pool> importPools; // pools from which types are imported
		std::vector<Pool> exportPools; // pools from which types are exported

		void addImportPool(Pool pool);
		void addExportPool(Pool pool);
		void addPool(Pool pool); // adds as both inport and export

		std::optional<Type> importType(size_t id) const; // searches the input pools
		std::optional<Type> importType(TypeID id) const; // searches the input pools. SSS is irrelevant
		void exportType(TypeID id, const Type& type);
		void exportType(const std::pair<TypeID, const Type&>& pair);

		std::optional<Type> parse(MemoryDataStream& mem, const std::shared_ptr<PoolType>& localPool, const std::shared_ptr<VTypeInfo>& vtypeinfo); // working on it
		static unsigned char getVTypeSizeSpec(unsigned char vtype); // returns the sss (0-3) of a virtual type
		static VTypeInfo::TypeID getFullTypeID(UINT_32 id, bool replacesOldID);
		static std::pair<UINT_32, bool> makeFullTypeID(size_t fullTypeID);

		Pool operator=(Pool pool); // clears all pools, then adds 'pool' as both import and export
		Pool operator+=(Pool pool); // adds 'pool' as both import and export

		PoolCollection() = default;
		PoolCollection(Pool pool); // adds 'pool' as import and export
		PoolCollection(Pool importPool, Pool exportPool);

		// create a new pool
		static Pool makePool(); // destroyed automatically by shared_ptr
	};
	

	// not exported into the dll
	struct ParseInfo
	{
		std::optional<Type> importType(size_t id) const; // searches the input pools. Same as PoolCollection::import but searches in the localPool first
		std::optional<Type> importType(PoolCollection::TypeID id) const; // searches the input pools. SSS is irrelevant. Same as PoolCollection::import but searches in the localPool first

		bool containsInvalidPointer() const;
		std::shared_ptr<PoolType>& guarateeLocalPool(); // make sure localPool points to a valid pool. Returns a reference to the shared_ptr. Referece becomes invalid if the ParseInfo object is destroyed.
		std::shared_ptr<PoolCollection>& guaranteePoolCollection(); // make sure pools points to a valid PoolCollection. Returns a reference to the shared_ptr. Referece becomes invalid if the ParseInfo object is destroyed.

		std::shared_ptr<PoolCollection> pools;
		std::shared_ptr<PoolType> localPool;
		std::shared_ptr<VTypeInfo> vtypeinfo;
		ParseInfo(std::shared_ptr<PoolCollection>& pools, std::shared_ptr<PoolType>& localPool, std::shared_ptr<VTypeInfo>& vtypeinfo);
		ParseInfo(const std::shared_ptr<PoolCollection>& pools, const std::shared_ptr<PoolType>& localPool, const std::shared_ptr<VTypeInfo>& vtypeinfo);
	};

	// not exported into the dll
	struct ConstParseInfo
	{
		std::shared_ptr<const PoolCollection> pools;
		std::shared_ptr<const PoolType> localPool;
		std::shared_ptr<const VTypeInfo> vtypeinfo;
		ConstParseInfo(const std::shared_ptr<const PoolCollection>& pools, const std::shared_ptr<const PoolType>& localPool, const std::shared_ptr<const VTypeInfo>& vtypeinfo);
	};
	// both shared_ptrs are used as optional (nullptr if no value)

	std::shared_ptr<PoolType> localPool; // file scope. only created for the root element
	std::shared_ptr<VTypeInfo> vtypeinfo; // holds virtual type information. only created for the root element

	// random constants
	//static constexpr unsigned char PRECISION_FLAG_CONVERSION_NEEDED = 0b10000000;

	// Specifiers

	class DF_API AbstractDataObject
	{
	public:
		virtual void saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const = 0;
		virtual void loadFromMemory(MemoryDataStream& mem, const ParseInfo& parseInfo) = 0;
	};

	class DF_API TypeSpecifier : public AbstractDataObject // The size specifier specifier is removed automatically when impicitely used.
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
		bool isVirtual() const;
		static bool isVirtual(unsigned char type);
		static bool isBuiltIn(unsigned char type);

		void saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const override;
		void loadFromMemory(MemoryDataStream& mem, const ParseInfo& parseInfo) override;

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
		static constexpr Type FLAG_RENAMED = 0b0010'0000;
		static constexpr Type FLAG_USETYPE = 0b0010'0000;
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
		static constexpr Type FLAG_VTYPE = 9;
		static constexpr Type FLAG_EXPORT = 0xA;
		static constexpr Type FLAG_IMPORT = 0xB;
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
		static constexpr Type DEFTYPE = FLAG_VTYPE;
		static constexpr Type USETYPE = FLAG_VTYPE | FLAG_USETYPE;
		static constexpr Type EXPORT = FLAG_EXPORT;
		static constexpr Type EXPORTAS = FLAG_EXPORT | FLAG_RENAMED;
		static constexpr Type IMPORT = FLAG_IMPORT;
		static constexpr Type IMPORTAS = FLAG_IMPORT | FLAG_RENAMED;
		static constexpr Type LAST_SPEC = IMPORT; // last spec that doesn't use extra bit
	};

	struct DF_API SizeSpecifier : public AbstractDataObject
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
		void saveToMemory(MemoryDataStream& stream, const ConstParseInfo& parseInfo) const override; // implicit size, size specifier specifier returned by sizeSpecifierSpecifier
		void loadFromMemory(MemoryDataStream& mem, const ParseInfo& parseInfo) override; // must not be used, will throw immediatly. This is because a sizeSpecifierSpecifier is needed to load a sizeSpecifier properlys. Maybe make this safer in the future TODO
		SizeSpecifier();
	};

	enum class DF_API TypeClass
	{
		Int,
		Float,
		String,
		List,
		Object
	};

	// forward declarations
	struct DF_API FixedArraySpecifier;
	struct DF_API MixedArraySpecifier;
	struct DF_API FixedObjectSpecifier;
	struct DF_API MixedObjectSpecifier;

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
	struct DF_API VariantTypeConversionError : std::runtime_error { VariantTypeConversionError(std::string message = "Invalid Conversion from type [NULL] or [undefined] to ODF::VariantType"); };

	typedef std::variant<MixedObjectSpecifier, FixedObjectSpecifier> ObjectSpecifier;
	typedef std::variant<MixedArraySpecifier, FixedArraySpecifier> ArraySpecifier;
	struct DF_API Type // a full type header
	{ // pointers are used as optionals. Deallocated in destructor
		TypeSpecifier type; // Note that immutable must be false to change this. If write-accessing this member manually, a check to immutable is needed
		bool immutable; // if true, an InvalidTypeMutation is thrown if a non-const member fucntion is called
		typedef std::variant<ObjectSpecifier, ArraySpecifier> ComplexSpecifier;
		ComplexSpecifier* complexSpec; // holds object specification optionallly

		void makeImmutable();
		void checkImmutable() const; // THrows an exception if immutable. Ignore that if type == NULL. The NULL type also expresses an "uninitialized" type that can be written.
		void readTypeSpecifier(MemoryDataStream& mem); // loads the type specifier from a stream and performs mutation check
		void clear(); // deletes every optional specifier
		Status saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const;
		Status loadFromMemory(MemoryDataStream& mem, const ParseInfo& parseInfo);
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

		bool isConvertableTo(const Type& other) const;
		bool isUnsigned() const; // returs false if not an interger type
		unsigned char getSize() const; // returns the size in bytes for integer and floating point types, 1 for cstr, 2 for wstr, and 3 for complex types
		void setFixType(const Type& type);
		const Type* getFixType() const; // returns the fixtype if the current type is complex and fixed. otherwise returns nullptr
		TypeClass getTypeClass() const;
		const std::vector<Type>* getArrayTypes() const; // returns the types vector of the mixed array specifier if mixed, otherwise nullptr
		const std::unordered_map<std::string, Type>* getObjectTypes() const; // returns the types vector of the mixed object specifier if mixed, otherwise nullptr
		void setSSS(unsigned char sss);
		void destroyComplexSpec();
		void makeComplexSpec(); // memory leak safe. Makes sure complexSpec is valid, keeps the old one if one is already existing
		void resetComplexSpec(); // memory leak safe. If an old complexSpec exists it gets destroyed, then a fresh one is allcated. Maybe fix performace of this by keeping and overwriting the old object? TODO

		friend bool DF_API operator!=(const Type& t1, const Type& t2);
		bool operator!(); // returns !(type != NULL) -> type == NULL
		bool operator==(const Type& other) const;

		operator unsigned char();
		Type& operator=(const Type& other);
		TypeSpecifier operator=(TypeSpecifier type);

		Type();
		Type(MemoryDataStream& mem, const ParseInfo& parseInfo); // loads the current object form mem. Throws a ODF::Status if loadFromMemory fails.
		Type(const Type& other);
		Type(Type&& type) noexcept;
		Type(TypeSpecifier type);
		~Type();

		static Type findBestType(TypeClass tc, unsigned char size, bool unsign = false); // only valid for primitive types. unsign is ignored if no integer type
	};
	// The ArraySpecifier holds all type information that is needed to load or save an array.
	// saves or loads the size specifier on save
	struct DF_API FixedArraySpecifier : public AbstractDataObject
	{
		SizeSpecifier size;
		Type fixType; // always holds a value, is just a pointer because of the forward declaring
		void saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const override;
		void loadFromMemory(MemoryDataStream& mem, const ParseInfo& parseInfo) override;
	};

	struct DF_API MixedArraySpecifier : public AbstractDataObject
	{
		std::vector<Type> types;

		void saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const override;
		void loadFromMemory(MemoryDataStream& mem, const ParseInfo& parseInfo) override;
	};

	struct DF_API FixedObjectSpecifier : public AbstractDataObject
	{
		Type fixType;
		std::vector<std::string> keys;
		void saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const override;
		void loadFromMemory(MemoryDataStream& mem, const ParseInfo& parseInfo) override;
	};

	struct DF_API MixedObjectSpecifier : public AbstractDataObject
	{
		typedef const std::pair<std::string, Type>& ObjectIterator;
		std::unordered_map<std::string, Type> properties; // use an ordered map to ensure iteration order is always the same.
		std::vector<std::string> iterationOrder;
		void saveToMemory(MemoryDataStream& mem, const ConstParseInfo& parseInfo) const override;
		void loadFromMemory(MemoryDataStream& mem, const ParseInfo& parseInfo) override;
	};

	// Complex data object functions
	class DF_API AbstractComplexDataObject
	{
	public:
		virtual void makeMixed() = 0;
		virtual Type canBeFixed() const = 0; // returns NULLTYPE if not fixable and UNDEFINED if empty
		virtual bool tryFixing() = 0;
		virtual bool isMixed() const = 0;
		virtual Type getFixedDatatype() const = 0;
	};

	class DF_API AbstractObject
	{
	public:
		typedef std::pair<const std::string, ODF> Pair;
		typedef std::unordered_map<std::string, ODF>::const_iterator ConstIterator;
		typedef std::unordered_map<std::string, ODF>::iterator Iterator;
		typedef std::unordered_map<std::string, ODF>::const_iterator::value_type IteratorType;

		virtual void insert(const std::string& key, const ODF& odf) = 0;
		virtual void insert(const Pair& value) = 0;
		virtual void insert(const std::map<std::string, ODF>& map) = 0;
		virtual void insert(const std::unordered_map<std::string, ODF>& umap) = 0;
		virtual void insert(const std::vector<Pair>& pairs) = 0;
		virtual void insert(const std::initializer_list<Pair>& pairs) = 0;
		virtual void erase(const std::string& key) = 0;

		virtual bool contains(const std::string& key) const = 0;
		virtual size_t count(const std::string& key) const = 0;
		virtual size_t size() const = 0;
		virtual void clear() = 0;

		virtual ODF& operator[](const std::string& key) = 0;
		virtual ODF& at(const std::string& key) = 0;
		virtual const ODF& at(const std::string& key) const = 0;

		virtual ConstIterator begin() const noexcept = 0;
		virtual Iterator begin() noexcept = 0;
		virtual ConstIterator end() const noexcept = 0;
		virtual Iterator end() noexcept = 0;
		virtual std::unordered_map<std::string, ODF>& getObjectContainer() = 0;

		// order:
		// update the keys, save the specifier, save the object.
		// or loadFromMemory with specifier (load specifier, then load object)
		virtual void updateKeys(ObjectSpecifier& spec) const = 0;
		virtual void saveToMemory(MemoryDataStream& mem, const ObjectSpecifier& spec) const = 0;
		virtual void loadFromMemory(MemoryDataStream& mem, const ObjectSpecifier& spec) = 0;
	};

	// The just like with the Array, the AbstractObject is an implementation of the MixedAObject. The MixedObject inherits all functions from it and adds loadFromMemory
	// The FIxedObject inherits the functions of the AbstractObjects, adds a loadFromMemory and overwrites all isnertion and access functions to perform type checks
	class DF_API Object :  public AbstractObject, public AbstractComplexDataObject
	{
	public:
		class DF_API FixedObject : public AbstractObject
		{
			std::unordered_map<std::string, ODF> map;
			Type fixType;
		public:
			void insert(const std::string& key, const ODF& odf) override;
			void insert(const Pair& value) override;
			void insert(const std::map<std::string, ODF>& map) override;
			void insert(const std::unordered_map<std::string, ODF>& umap) override;
			void insert(const std::vector<Pair>& pairs) override;
			void insert(const std::initializer_list<Pair>& pairs) override;
			virtual void erase(const std::string& key) override;

			bool contains(const std::string& key) const override;
			size_t count(const std::string& key) const override;
			size_t size() const override;
			void clear() override;

			ODF& operator[](const std::string& key) override; // also overwritten by FixedObject, as it could insert elements
			ODF& at(const std::string& key) override;
			const ODF& at(const std::string& key) const override;

			ConstIterator begin() const noexcept override;
			Iterator begin() noexcept override;
			ConstIterator end() const noexcept override;
			Iterator end() noexcept override;
			std::unordered_map<std::string, ODF>& getObjectContainer() override;

			void setType(const Type& type); bool isConvertableTo(const Type& newFixType) const; // newFixType must be the new fix type, aka the type that is contained in the array, not the array type itself
			bool convertTo(const Type& type);
			bool convertTo(std::function<Type(const std::unordered_map<std::string, ODF>&)> type, std::function<void(ODF& odf)> converter);
			const Type& getType() const; // returns the type of the elements contained in the object
			
			void updateKeys(ObjectSpecifier& spec) const override;
			void saveToMemory(MemoryDataStream& mem, const ObjectSpecifier& spec) const override;
			void loadFromMemory(MemoryDataStream& mem, const ObjectSpecifier& spec) override;

			FixedObject();
			template<typename T>
			FixedObject(const std::initializer_list<std::pair<std::string, T>>& initializer_list)
			{
				for (const std::pair<std::string, T>& it : initializer_list)
					insert(it);
			}
			template<typename T>
			FixedObject(const std::map<std::string, T>& map)
			{
				for (const std::pair<std::string, T>& it : map)
					insert(it);
			}
			template<typename T>
			FixedObject(const std::unordered_map<std::string, T>& umap)
			{
				for (const std::pair<std::string, T>& it : umap)
					insert(it);
			}
			template<typename T>
			FixedObject(const std::vector<std::pair<std::string, T>>& pairs)
			{
				for (const std::pair<std::string, T>& it : pairs)
					insert(it);
			}
			template<typename T>
			FixedObject(const std::initializer_list<std::variant<std::pair<std::string, T>, ComplexExplicitor::FOBJSTRUCT>>& pairs)
			{
				for (const std::variant<std::pair<std::string, T>, ComplexExplicitor>& it : pairs)
					if (!it.index())
						insert(it);
			}

			friend class ODF::Object;
		};

		class DF_API MixedObject : public AbstractObject
		{
			std::unordered_map<std::string, ODF> map;
		public:
			void insert(const std::string& key, const ODF& odf) override;
			void insert(const Pair& value) override;
			void insert(const std::map<std::string, ODF>& map) override;
			void insert(const std::unordered_map<std::string, ODF>& umap) override;
			void insert(const std::vector<Pair>& pairs) override;
			void insert(const std::initializer_list<Pair>& pairs) override;
			void erase(const std::string& key) override;

			bool contains(const std::string& key) const override;
			size_t count(const std::string& key) const override;
			size_t size() const override;
			void clear() override;

			ODF& operator[](const std::string& key) override;
			ODF& at(const std::string& key) override;
			const ODF& at(const std::string& key) const override;

			ConstIterator begin() const noexcept override;
			Iterator begin() noexcept override;
			ConstIterator end() const noexcept override;
			Iterator end() noexcept override;
			std::unordered_map<std::string, ODF>& getObjectContainer() override;

			void updateKeys(ObjectSpecifier& spec) const override;
			void saveToMemory(MemoryDataStream& mem, const ObjectSpecifier& spec) const override;
			void loadFromMemory(MemoryDataStream& mem, const ObjectSpecifier& spec) override;

			MixedObject();
			MixedObject(const std::initializer_list<Pair>& elements);
			MixedObject(const std::initializer_list<std::variant<Pair, ComplexExplicitor::OBJSTRUCT>>& pairs);

			friend class ODF::Object;
		};
		
		std::variant<FixedObject, MixedObject> object;

		void insert(const std::string& key, const ODF& odf) override;
		void insert(const Pair& value) override;
		void insert(const std::map<std::string, ODF>& map) override;
		void insert(const std::unordered_map<std::string, ODF>& umap) override;
		void insert(const std::vector<Pair>& pairs) override;
		void insert(const std::initializer_list<Pair>& pairs) override;
		virtual void erase(const std::string& key);

		bool contains(const std::string& key) const override;
		size_t count(const std::string& key) const override;
		size_t size() const override;
		void clear() override;

		ODF& operator[](const std::string& key) override; // also overwritten by FixedObject, as it could insert elements
		ODF& at(const std::string& key) override;
		const ODF& at(const std::string& key) const override;

		ConstIterator begin() const noexcept override;
		Iterator begin() noexcept override;
		ConstIterator end() const noexcept override;
		Iterator end() noexcept override;
		std::unordered_map<std::string, ODF>& getObjectContainer() override;

		void updateKeys(ObjectSpecifier& spec) const override;
		void saveToMemory(MemoryDataStream& mem, const ObjectSpecifier& spec) const override;
		void loadFromMemory(MemoryDataStream& mem, const ObjectSpecifier& spec) override;

		void clearAndFix(); // clear and make fixed
		void clearAndFix(const Type& elementType); // clear and make fixed
		void clearAndMix(); // clear and make mixed
		
		void makeMixed() override; // TODO
		Type canBeFixed() const override; // TODO
		bool tryFixing() override; // returns NULLTYPE if not fixable and UNDEFINED if empty
		bool isMixed() const override; // TODO
		Type getFixedDatatype() const override; // called on fixed list, returns the actual datatype that every element has. Throws std::bad_variant_access if called on mixed
	
		FixedObject& fixed();
		MixedObject& mixed();

		friend bool DF_API operator==(const Object& obj1, const Object& obj2);
		friend bool DF_API operator!=(const Object& obj1, const Object& obj2);

		Object& operator=(const Object& other);
		Object& operator=(const Object::MixedObject& mobj);
		Object& operator=(const std::initializer_list<std::variant<Pair, ComplexExplicitor::OBJSTRUCT>>& pairs);
		Object& operator=(const Object::FixedObject& fobj);
		template<typename T> Object& operator=(const std::initializer_list<std::variant<std::pair<std::string, T>, ComplexExplicitor::FOBJSTRUCT>>& pairs) {
			object.emplace<FixedObject>() = FixedObject(pairs);
			return *this;
		} // templated sigma rizz
	
		Object();
		Object(const Object& other);
		Object(const Object::MixedObject& mobj);
		Object(const std::initializer_list<std::variant<Pair, ComplexExplicitor::OBJSTRUCT>>& pairs);
		Object(const Object::FixedObject& fobj);
		template<typename T> Object(const std::initializer_list<std::variant<std::pair<std::string, T>, ComplexExplicitor::FOBJSTRUCT>>& pairs) { *this = FixedObject(pairs); }
	};


	// The AbstractArray is an implementation of the mixed array.
	// The mixedArray inherits everything from it.
	// The FixedArray inherits erase, etc. and overwrites the push and insert functions to make a fixed check
	class DF_API AbstractArray
	{
	public:
		typedef std::vector<ODF>::const_iterator ConstIterator;
		typedef std::vector<ODF>::iterator Iterator;
		typedef std::vector<ODF>::const_iterator::value_type IteratorType;

		virtual void push_back(const ODF& odf) = 0;
		virtual void push_front(const ODF& odf) = 0;
		virtual void erase(size_t index) = 0;
		virtual void erase(size_t index, size_t numberOfElements) = 0;
		virtual void erase_range(size_t from, size_t to) = 0;
		virtual void insert(size_t where, const ODF& odf) = 0;

		virtual std::optional<size_t> find(const ODF& odf) const = 0;
		virtual size_t size() const = 0;
		virtual void clear() = 0;
		virtual void resize(size_t newSize) = 0;

		virtual ODF& operator[](size_t index) = 0;
		virtual const ODF& operator[](size_t index) const = 0;
		virtual ODF& at(size_t index) = 0;
		virtual const ODF& at(size_t index) const = 0;

		virtual ConstIterator begin() const noexcept = 0;
		virtual Iterator begin() noexcept = 0;
		virtual ConstIterator end() const noexcept = 0;
		virtual Iterator end() noexcept = 0;
		virtual std::vector<ODF>& getArrayContainer() = 0;

		// order:
		// update the keys, save the specifier, save the object.
		// or loadFromMemory with specifier (load specifier, then load object)
		virtual void updateSpec(ArraySpecifier& spec) const = 0;
		virtual void saveToMemory(MemoryDataStream& mem) const = 0;
		virtual void loadFromMemory(MemoryDataStream& mem, const ArraySpecifier& spec) = 0;
	};
	
	class DF_API List : public AbstractComplexDataObject, public AbstractArray
	{
	public:
		class DF_API FixedArray : public AbstractArray
		{
			std::vector<ODF> list;
			Type fixType; // uses a complex type to compare. Example: A list made out of FixedArrays holding different types would have the same Variant- and Primitive- type but different types in the final document
		public:
			void push_back(const ODF& odf) override;
			void push_front(const ODF& odf) override;
			void erase(size_t index) override;
			void erase(size_t index, size_t numberOfElements) override;
			void erase_range(size_t from, size_t to) override;
			void insert(size_t where, const ODF& odf) override;

			std::optional<size_t> find(const ODF& odf) const override;
			size_t size() const override;
			void clear() override;
			void resize(size_t newSize) override;

			ODF& operator[](size_t index) override;
			const ODF& operator[](size_t index) const override;
			ODF& at(size_t index) override;
			const ODF& at(size_t index) const override;

			ConstIterator begin() const noexcept override;
			Iterator begin() noexcept override;
			ConstIterator end() const noexcept override;
			Iterator end() noexcept override;
			std::vector<ODF>& getArrayContainer() override;

			void updateSize(ODF::Type& type) const;
			void setType(const Type& type); // can only be called on an empty list, otherwise throws exception // TODO
			bool isConvertableTo(const Type& newFixType) const; // newFixType must be the new fix type, aka the type that is contained in the array, not the array type itself
			bool convertTo(const Type& type);
			bool convertTo(std::function<Type(const std::vector<ODF>&)> type, std::function<void(ODF& odf)> converter);
			const Type& getType() const;

			void updateSpec(ArraySpecifier& spec) const override;
			void saveToMemory(MemoryDataStream& mem) const override;
			void loadFromMemory(MemoryDataStream& mem, const ArraySpecifier& spec) override;

			struct InvalidTypeChange : public std::runtime_error
			{
				InvalidTypeChange(std::string message = "Error: ODF::Array::FixedArray::setType called on non empty array");
			};

			FixedArray();
			template<typename T>
			FixedArray(const std::initializer_list<T>& initializer_list)
			{
				for (const T& t : initializer_list)
					push_back(t);
			}
			template<typename T>
			FixedArray(const std::vector<T>& vector)
			{
				for (const T& t : vector)
					push_back(t);
			}
		};

		// inherits all vector functions from AbstractArray
		class DF_API MixedArray : public AbstractArray
		{
			std::vector<ODF> list;
		public:
			void push_back(const ODF& odf) override;
			void push_front(const ODF& odf) override;
			void erase(size_t index) override;
			void erase(size_t index, size_t numberOfElements) override;
			void erase_range(size_t from, size_t to) override;
			void insert(size_t where, const ODF& odf) override;

			std::optional<size_t> find(const ODF& odf) const override;
			size_t size() const override;
			void clear() override;
			void resize(size_t newSize) override;

			ODF& operator[](size_t index) override;
			const ODF& operator[](size_t index) const override;
			ODF& at(size_t index) override;
			const ODF& at(size_t index) const override;

			ConstIterator begin() const noexcept override;
			Iterator begin() noexcept override;
			ConstIterator end() const noexcept override;
			Iterator end() noexcept override;
			std::vector<ODF>& getArrayContainer() override;

			void updateSpec(ArraySpecifier& spec) const override;
			void saveToMemory(MemoryDataStream& mem) const override;
			void loadFromMemory(MemoryDataStream& mem, const ArraySpecifier& spec) override;

			MixedArray();
			MixedArray(const std::initializer_list<ODF>& initializer_list);
		};

		std::variant<FixedArray, MixedArray> list;

		void push_back(const ODF& odf) override;
		void push_front(const ODF& odf) override;
		void erase(size_t index) override;
		void erase(size_t index, size_t numberOfElements) override;
		void erase_range(size_t from, size_t to) override;
		void insert(size_t where, const ODF& odf) override;

		std::optional<size_t> find(const ODF& odf) const override;
		size_t size() const override;
		void clear() override;
		void resize(size_t newSize) override;

		ODF& operator[](size_t index) override;
		const ODF& operator[](size_t index) const override;
		ODF& at(size_t index) override;
		const ODF& at(size_t index) const override;

		ConstIterator begin() const noexcept override;
		Iterator begin() noexcept override;
		ConstIterator end() const noexcept override;
		Iterator end() noexcept override;
		std::vector<ODF>& getArrayContainer() override;

		void updateSpec(ArraySpecifier& spec) const override;
		void saveToMemory(MemoryDataStream& mem) const override;
		void loadFromMemory(MemoryDataStream& mem, const ArraySpecifier& spec) override;

		// type functions
		void clearAndFix(); // clear and make fixed
		void clearAndFix(const Type& elementType); // clear and make fixed
		void clearAndMix(); // clear and make mixed

		void makeMixed() override; // TODO
		Type canBeFixed() const override; // returns NULLTYPE if not fixable and UNDEFINED if empty
		bool tryFixing() override; // TODO
		bool isMixed() const override; // TODO
		Type getFixedDatatype() const override; // called on fixed list, returns the actual datatype that every element has. Throws std::bad_variant_access if called on mixed

		FixedArray& fixed() const; // just forwards to get<FixedArray>()
		MixedArray& mixed() const; // just forwards to get<MixedArray>()

		void resetAndSetSize(size_t newSize, const std::vector<Type>& newTypes); // this function is called by the root object which loads the specifier to tell the array their size. All current data is lost.
		void resetAndSetSize(size_t newSize, const Type& newType); // this function is called by the root object which loads the specifier to tell the array their size. All current data is lost. Works only on fixed arrays

		friend bool DF_API operator==(const List& arr1, const List& arr2);
		friend bool DF_API operator!=(const List& arr1, const List& arr2);
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
		template<typename T> List(const std::initializer_list<T>& initializer_list) : FixedArray(initializer_list) {}
		template<typename T> List(const std::vector<T>& vector) : FixedArray(vector) {}
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

	// Pool interface: do not use pools by default to increase performance on temporarily created objects or if they aren't needed.
	std::shared_ptr<PoolCollection> pools;
	void usePools(); // creates a valid value in the optional
	void resetPools(); // remove all pools (deallocate pools)
	void addPool(Pool pool);
	void addImportPool(Pool pool);
	void addExportPool(Pool pool);
	Pool operator+=(Pool pool); // adds the pool as import and export pool
	const PoolCollection& operator=(const PoolCollection& pc);
	ODF(const PoolCollection& pc);

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
	ODF& operator=(const char* str);
	ODF& operator=(const std::wstring& wstr);
	ODF& operator=(const wchar_t* wstr);
	// array types
	ODF& operator=(const List& arr);
	ODF& operator=(const List::MixedArray& marr);
	ODF& operator=(const std::initializer_list<ODF>& initializer_list);
	ODF& operator=(const List::FixedArray& farr);
	ODF& operator=(const Object& obj);
	ODF& operator=(const Object::MixedObject& mobj);
	ODF& operator=(const std::initializer_list<std::variant<Object::Pair, ComplexExplicitor::OBJSTRUCT>>& initializer_list);
	ODF& operator=(const Object::FixedObject& fobj);
	template<typename T> ODF& operator=(const std::initializer_list<std::variant<std::pair<std::string, T>, ComplexExplicitor::FOBJSTRUCT>>& initializer_list) { *this = Object::FixedObject(initializer_list); }
	template<typename T> ODF& operator=(const std::initializer_list<T>& initializer_list) { return *this = List::FixedArray(initializer_list); }
	
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
	ODF(const Object::MixedObject& mobj);
	ODF(const std::initializer_list<std::variant<Object::Pair, ComplexExplicitor::OBJSTRUCT>>& initializer_list);
	ODF(const Object::FixedObject& fobj);
	
	ODF(const List& arr);
	ODF(const List::MixedArray& marr);
	ODF(const std::initializer_list<ODF>& initializer_list);
	ODF(const List::FixedArray& farr);
	template<typename T> ODF(const std::initializer_list<T>& initializer_list) : ODF(List::FixedArray(initializer_list)) {} // although a (ODF(Array(...)) would be sufficient, the fixed array is needed to explicitely invoke the ODF(FixedArray) constructor which explicitely calls operator=(const FixedArray&) which updates the size specifier
	template<typename T> ODF(const std::vector<T>& vector) : ODF(List::FixedArray(vector)) {}
	template<typename T> ODF(const std::initializer_list<std::variant<std::pair<std::string, T>, ComplexExplicitor::FOBJSTRUCT>>& initializer_list) { *this = initializer_list;  }
	template<typename T>
	ODF(const std::map<std::string, T>& map)
	{
		for (const std::pair<std::string, T>& it : map)
			insert(it);
	}
	template<typename T>
	ODF(const std::unordered_map<std::string, T>& umap)
	{
		for (const std::pair<std::string, T>& it : umap)
			insert(it);
	}
	template<typename T>
	ODF(const std::vector<std::pair<std::string, T>>& pairs)
	{
		for (const std::pair<std::string, T>& it : pairs)
			insert(it);
	}

	// ostream printing
	// print flags
	static bool printType;
#ifdef ODF_THREADSAFE
	static std::mutex printMutex;
#endif


	// print operators
	friend std::ostream& operator<<(std::ostream& out, const ODF& odf);
	friend std::ostream& operator<<(std::ostream& out, const Object& obj);
	friend std::ostream& operator<<(std::ostream& out, const List& list);
	friend std::ostream& operator<<(std::ostream& out, const std::wstring& wstr);
	friend std::ostream& operator<<(std::ostream& out, const Type& type);

	// print functios, higher level than operators because of PrettyPrintInfo. PrettyPrintInfo is not taken by reference or shared_ptr on purpose as it is very small and modified in almost every stack movement (indentation)
	static std::ostream& print(std::ostream& out, const ODF& odf, PrettyPrintInfo ppi);
	static std::ostream& print(std::ostream& out, const Object& obj, PrettyPrintInfo ppi);
	static std::ostream& print(std::ostream& out, const List& list, PrettyPrintInfo ppi);
	static std::ostream& print(std::ostream& out, const Type& type, PrettyPrintInfo ppi);

	// generic functions
	ODF& immutable(); // makes the object type immutable and returns a reference to itself. Used in the implementation of access operators
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
	void makeList(); // clears content and converts element to a mixed list
	void makeList(const Type& elementType); // clears content and converts to fxlist of with elementtype
	void makeList(const TypeSpecifier& elementType); // clears content and converts to fxlist of with elementtype
	void push_back(const ODF& odf);
	void push_front(const ODF& odf);
	void insert(size_t where, const ODF& odf);
	void erase(size_t index);
	void erase(size_t index, size_t numberOfElements);
	void erase_range(size_t from, size_t to);
	std::optional<size_t> find(const ODF& odf);
	bool push_back_nothrow(const ODF& odf) noexcept;
	bool push_front_nothrow(const ODF& odf) noexcept;
	bool erase_nothrow(size_t index) noexcept;
	bool erase_nothrow(size_t index, size_t numberOfElements) noexcept;
	bool erase_range_nothrow(size_t from, size_t to) noexcept;
	bool insert_nothrow(size_t where, const ODF& odf) noexcept;
	std::optional<size_t> find_nothrow(const ODF& odf) noexcept;

	// object functions
	using Pair = Object::Pair;
	void makeObject(); // clears content and converts element to a mixed object
	void makeObject(const Type& elementType); // clears content and converts to mxobj with fixType = elementType
	void makeObject(const TypeSpecifier& elementType); // clears content and converts to mxobj with fixType = elementType
	virtual void insert(const std::string& key, const ODF& odf);
	virtual void insert(const Pair& value);
	virtual void insert(const std::map<std::string, ODF>& map);
	virtual void insert(const std::unordered_map<std::string, ODF>& umap);
	virtual void insert(const std::vector<Pair>& pairs);
	virtual void insert(const std::initializer_list<Pair>& pairs);
	virtual void erase(const std::string& key);
	virtual bool contains(const std::string& key) const;
	virtual size_t count(const std::string& key) const;

	// list and object functions
	size_t size() const;
	void clear();

	// access operators
	ODF& at(size_t index); // throws bad_variant_access if called on not an array
	const ODF& at(size_t index) const; // throws bad_variant_access if called on not an array
	ODF& at(const std::string& key); // throws bad_variant_access if called on a object
	const ODF& at(const std::string& key) const; // throws bad_variant_access if called on a object
	ODF& operator[](size_t index); // throws bad_variant_access if called on not an array
	ODF& operator[](std::string_view key); // throws bad_variant_access if called on a object

	// visitors (replacement for Iterators for now, and for advanced or accelerated processing directly in the object)
	template<typename Func> void visit(Func&& functor)
	{
		std::visit([&](auto& container) {
			using decaytype = std::decay_t<decltype(container)>;

			if constexpr (std::is_same_v<decaytype, List>)
			{
				List& list = std::get<List>(content);
				for (ODF& odf : list)
					functor(odf);
			}
			else if constexpr (std::is_same_v<decaytype, List::FixedArray> || std::is_same_v<decaytype, List::MixedArray>)
			{
				decaytype& list = std::get<decaytype>(std::get<List>(content).list);
				for (ODF& odf : list)
					functor(odf);
			}
			else if constexpr (std::is_same_v<decaytype, Object>)
			{
				Object& obj = std::get<Object>(content);
				for (Pair& pair : obj)
					functor(pair);
			}
			else if constexpr (std::is_same_v<decaytype, Object::FixedObject> || std::is_same_v<decaytype, Object::MixedObject>)
			{
				decaytype& obj = std::get<decaytype>(std::get<Object>(content).object);
				for (Pair& pair : obj)
					functor(pair);
			}
			else
			{
				std::visit([](const ODF& odf) {
					functor(odf);
				}, content);
			}
		}, content);
	}

	// Operators
	friend bool operator== (const ODF& odf1, const ODF& odf2); // TODO

	// load and save functions
	Status saveToMemory(MemoryDataStream& mem) const; // main save function
	Status saveToMemory(char* destination, size_t size) const;
	Status loadFromMemory(MemoryDataStream& mem); // main load function
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

public:
	// predefined Types:

	static ComplexExplicitor::OBJSTRUCT __MOBJ;
	static ComplexExplicitor::FOBJSTRUCT __FOBJ;
};

using std::literals::string_literals::operator""s;
