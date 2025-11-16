#pragma once
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

#include "GenericDebug.h"

#undef NULL

#define TYPE(T) Data((T)0).index()

#ifndef USE_FIXED_TYPES
#define INT_8 char
#define UINT_8 unsigned char
#define INT_16 short
#define UINT_16 unsigned short
#define INT_32 int
#define UINT_32 unsigned int
#define INT_64 long
#define UINT_64 unsigned long
#else
#define INT_8 int8_t
#define UINT_8 uint8_t
#define INT_16 int16_t
#define UINT_16 uint16_t
#define INT_32 int32_t
#define UINT_32 uint32_t
#define INT_64 int64_t
#define UINT_64 uint64_t
#endif

#define DISABLE_STRING_OBFUSCATION
#ifndef DISABLE_STRING_OBFUSCATION
constexpr bool enable_string_obfuscation_default = true;
#else
constexpr bool enable_string_obfuscation_default = false;
#endif

#ifndef DEV
#ifdef EXPORT
#define DF_API __declspec(dllexport)
#else
#define DF_API __declspec(dllimport)
#endif
#else
#define DF_API
#endif

// lets make a safe pointer quicky. Throws std::out_of_range if invalid access
// This class is a smart pointer, safe memory accessor, memory stream and encryption tool in just one class :)
class DF_API MemoryDataStream
{
	char* start;
	char* current;
	char* firstInvalidAddress;
	bool rangeDefined; // is current and firstInvalidAddress defined with a valid range?
	bool insecure; // skip access violation checks
	bool readOnly; // read only
	bool enableBytewiseEncryption; // call the encryption function for every read and written byte
	bool enablePreprocessing; // the preprocessing will be executed after the range was specified.
	bool enablePostprocessing; // the postprocessing will be executed when the destructor is called.
	bool deallocateOnDestruction; // deallocate the data on destruction. Carefull with that
	bool selfAllocated; // is the data allocated by exactly THIS object with allocate() ?
	bool writeFullStream; // if this is true and on destruction or call of finish() current != firstInvalidAddress, then throw an exception
	bool finished; // was finish() already called? if yes, then the destructor does nothing.
	bool obfuscated;

	static void makeUnreadable(std::string& str);
	static void makeReadable(std::string& str);
	static void makeUnreadable(std::wstring& wstr);
	static void makeReadable(std::wstring& wstr);

	void baseinit(); // called on every new initialization, like on allocation or construction

	std::function<void(char&)> encryptor;
	std::function<void(char&)> decryptor;
	std::function<void(char*, size_t)> preprocessor;
	std::function<void(char*, size_t)> postprocessor;
public:
	// use one of the range functions to set bounds
	void setDataPtr(char* data);
	void setDataPtr(const char* data);
	void setMaxSize(size_t size); // range function
	void setLastValidAddress(const char* lastValidAddr); // range function
	void setFirstInvalidAddress(const char* firstInvalidAddr); // range function
	void enableSecurity(bool on = true);
	void makeReadOnly();
	void setByteEncryptor(std::function<void(char&)> byteEncryptor); // key needs to be either hardcoded or passed via lampda capture
	void setByteDecryptor(std::function<void(char&)> byteDecryptor); // key needs to be either hardcoded or passed via lampda capture
	void disableByteEncryption();
	void setPreprocessor(std::function<void(char*, size_t)> preprocessorFunction); // key needs to be either hardcoded or passed via lampda capture
	void disablePreprocessing();
	void setPostprocessor(std::function<void(char*, size_t)> postprocessorFunction); // key needs to be either hardcoded or passed via lampda capture
	void disablePostprocessing();
	void deallocateDataOnDestruction(bool deallocate);
	void enableStringObfuscation(bool obfuscation = true);
	void disableStringObfuscation();
	void allocate(size_t size);
	void deallocate(); // explicitely deallocate data. Called in Destructor
	MemoryDataStream move(); // the current stream looses all its rights to the data and transfers it to a new one and returns it.
	void move(MemoryDataStream& mem); // just like the other move.
	void finish(); // basically the destructor. Call if the Stream is no longer needed. Handles memory deallocation

	void write(const char* bytes, size_t size);
	template<typename T>
	void write(T value)
	{
		write((char*)&value, sizeof(value));
	}

	std::string readStr();
	std::wstring readWstr();
	void writeStr(std::string str);
	void writeWstr(std::wstring wstr);

	unsigned char peek() const;
	void peek(char* destination, size_t size);
	char read();
	void read(char* destination, size_t size);
	char* read(size_t size);
	char* data();
	size_t size();
	size_t sizeLeft();
	template<typename T>
	T read()
	{
		T value{};
		read((char*)&value, sizeof(value));
		return value;
	}
	template<typename T>
	T peek()
	{
		T value{};
		peek((char*)&value, sizeof(value));
		return value;
	}

	MemoryDataStream(size_t size); // allocates own data
	MemoryDataStream(char* data);
	MemoryDataStream(const char* data);
	MemoryDataStream(char* data, char* firstInvalidAddr);
	MemoryDataStream(const char* data, const char* firstInvalidAddr);
	MemoryDataStream(char* data, size_t size);
	MemoryDataStream(const char* data, size_t size);
	~MemoryDataStream();

	// thrown if trying to write to a read only stream
	struct ReadOnlyViolation : public std::runtime_error
	{
		ReadOnlyViolation(std::string message);
	};
	// thrown if trying to write out of bounds in a secure stream
	struct AccessViolation : public std::runtime_error
	{
		AccessViolation(std::string message);
	};
	// thrown if the stream hasnt reached its end on finish() if writeFullStream is specified
	struct IncompleteWrite : public std::runtime_error
	{
		IncompleteWrite(std::string message);
	};
};

/// <summary>
/// The OOD class is a basic OOD object. It can contain more OOD objects, depending on the type of it, and load or safe itself into OODR or OOD into a string or file.
/// </summary>
class DF_API OOD
{
protected:
	bool forcecompression; // if true, the map or list must only contain 1 data type.
	bool forcetype; // if true, OOD is an element in a list or object that has forcecompression enabled, which means it is only allowed to have type "forcedType"
	size_t forcedType; // this will be the type index that is forced if an OOD is added to a non-mixed object or list
	size_t expectedType; // this is the type a non-mixed object or list must hold. It is specified in forceCompression

	bool enable_string_obfuscation;
	static bool enable_string_obfuscation_static;

	//this for file operations
	void makeUnreadable(std::string& str);
	void makeReadable(std::string& str);

	std::string readFile(std::string file, bool silent = false);
	bool writeFile(std::string file, const char* data, size_t size, bool silent = false); // if silent is true, the function will not print error messages to the cout
	
	void saveAsObject(MemoryDataStream& mem); // call only if actual type is an object type
	void saveAsList(MemoryDataStream& mem); // call only if actual type is an array type
public:
	typedef unsigned char TypeSpecifier;
	struct Type // enough for a primitive type
	{
		static constexpr TypeSpecifier CHAR = 0;
		static constexpr TypeSpecifier UCHAR = 1;
		static constexpr TypeSpecifier SHORT = 2;
		static constexpr TypeSpecifier USHORT = 3;
		static constexpr TypeSpecifier INT = 4;
		static constexpr TypeSpecifier UINT = 5;
		static constexpr TypeSpecifier LONG = 6;
		static constexpr TypeSpecifier ULONG = 7;
		static constexpr TypeSpecifier FLOAT = 8;
		static constexpr TypeSpecifier DOUBLE = 9;
		static constexpr TypeSpecifier CSTR = 10;
		static constexpr TypeSpecifier WSTR = 11;
		static constexpr TypeSpecifier FXOBJ = 16;
		static constexpr TypeSpecifier MXOBJ = 20;
		static constexpr TypeSpecifier FXLST = 24;
		static constexpr TypeSpecifier MXLST = 28;

		static constexpr TypeSpecifier Size_8bit = 0b00;
		static constexpr TypeSpecifier Size_16bit = 0b01;
		static constexpr TypeSpecifier Size_32bit = 0b10;
		static constexpr TypeSpecifier Size_64bit = 0b11;

		static constexpr TypeSpecifier NULL = 0x0; // bruh. Just for naming convention
		static constexpr TypeSpecifier END = 0xff;
	};

	//forward declarations
	struct MixedObjectSpecifier;
	struct FixedObjectSpecifier;
	typedef std::variant<MixedObjectSpecifier, FixedObjectSpecifier> ObjectSpecifier;
	static constexpr size_t VI_Mixed = 0; // variant index of ObjectSpecifier
	static constexpr size_t VI_Fixed = 1; // variant index of ObjectSpecifier
	struct SizeSpecifier;

	struct ComplexType
	{ // pointers are used as optionals. Deallocated in destructor
		TypeSpecifier type;
		ObjectSpecifier* obj;
		SizeSpecifier* size;

		void clear(); // deletes every optional specifier
		void saveToMemory(MemoryDataStream& mem) const;
		void setType(TypeSpecifier type); // use this, otherwise the object might throw exceptions or undefined behaiviour on save
		bool isPrimitive() const;
		bool isString() const;

		ComplexType();
		~ComplexType();
	};

	struct MixedObjectSpecifier
	{
		typedef std::pair<std::string, ComplexType> IteratorType;
		std::map<std::string, ComplexType*> properties; // use an ordered map to ensure iteration order is always the same.
		void saveToMemory(MemoryDataStream& mem) const;
	};

	struct FixedObjectSpecifier
	{
		ComplexType* header;
		std::vector<std::string> keys;
		void saveToMemory(MemoryDataStream& mem) const;
	};

	struct SizeSpecifier
	{
		static constexpr size_t OverflowSize8bit = 255;
		static constexpr size_t OverflowSize16bit = 65'536;
		static constexpr size_t OverflowSize32bit = 4'294'967'296;
		// no OverflowSize64bit, as it would be zero on a 64bit system
		size_t actualSize;

		TypeSpecifier sizeSpecifierSpecifier() const; // wow. Use like TypeSpecifier =	
		void load(MemoryDataStream& stream, TypeSpecifier type);
		void save(MemoryDataStream& stream, TypeSpecifier type) const; // explicit size. Only the last 2 bits matter.
		void save(MemoryDataStream& stream) const; // implicit size, size specifier specifier returned by sizeSpecifierSpecifier
	};

	bool isObjectArray(std::unordered_map<std::string, Type>* fields = nullptr) const; // if fields isnt nullptr, then it returns a ist of the keys with their type
	
	ComplexType header;
	//data
	//For basicly every non-complex type
	typedef std::variant <
		std::nullptr_t, // NULL, index 0
		//Basic types
		INT_8, // index 1
		UINT_8, // index 2
		INT_16, // index 3
		UINT_16, // index 4
		INT_32, // index 5
		UINT_32, // index 6
		INT_64, // index 7
		UINT_64, // index 8
		float, // index 9
		double, // index A
		std::string, // index B
		std::wstring, // index C
		//MLIST type
		std::vector<OOD>, // index D
		//MOBJ type
		std::unordered_map<std::string, OOD> // E
	> Data;

	//variant utilities
	static std::string getTypeStr(const size_t& data);

	//Exceptions
	class Exception : public std::exception
	{
	protected:
		std::string msg;
	public:
		const char* what() const noexcept override final;
	};

	// thrown if an invalid conversion happens like from std::string to int
	struct BadImplicitConversion : public Exception
	{
		BadImplicitConversion(size_t sourceType, size_t destType, std::string msg = "");
	};

	//currently not used. Thrown if an invalid type is added to a non-mixed object or list
	struct BadArgumentType : public Exception
	{
		BadArgumentType(size_t wantedArgumentType, size_t realArgumentType, std::string message = "");
		BadArgumentType(std::vector<size_t> wantedArgumentTypes, size_t realArgumentType, std::string message = "");
	};

	// Thrown if operator[] is called on a primitive type
	struct BadAccessOperator : public Exception
	{
		BadAccessOperator(std::string message = "");
	};
	struct BadConstructionType : public Exception
	{
		BadConstructionType(size_t wantedArgumentType, size_t realArgumentType, std::string message = "");
	};
	struct NotCompressable : public Exception
	{
		NotCompressable(std::string message = "Object not compressable: contains mixed types");
	};
	struct InvalidObjectArray : public Exception
	{
		InvalidObjectArray(std::string message = "Object Array was specified but holds invalid data.");
	};
	struct SwitchCaseDefaultReached : public Exception
	{
		SwitchCaseDefaultReached(std::string message = "mist :(");
	};
	struct InvalidArraySize : public Exception
	{
		InvalidArraySize(std::string message = "Array is to large");
	};

	Data data;

	//non template functions
	void enableStringObfuscation(bool justForThis);
	void disableStringObfuscation(bool justForThis);

	bool isList() const;
	bool isObject() const;
	void unforceCompression();

	bool loadFromMemory(const char* mem, size_t size); // returns a pointer to the byte after the last byte that was processed, that means the next byte if you would continue reading
	void loadFromMemory(MemoryDataStream& mem);
	void loadWithOutType(TypeSpecifier t, MemoryDataStream& mem);

	MemoryDataStream saveToStream();
	char* saveToMemory(size_t& size);
	void saveToMemory(MemoryDataStream& mem, bool withoutType = false); // dest is preallocated. there must be at least getNeededSize() free bytes. It returns the needed size just for convenience. You could use it like destination += ood.saveToMemory(destination);
	Type getSaveType() const;
	size_t getNeededSize() const; // returns how much memory is needed to save the object.
	size_t getPrimitiveDataSize(size_t type) const;
	size_t getSizeWithoutType() const;
	size_t getValueSize() const; // returns the size of only the values. This is needed to compute the size of an OBJLIST. WARNING: Used in wrong approach, deprecated/unused

	bool loadFromFile(std::string filename);
	bool saveToFile(std::string filename);
	
	template<typename T>
	void forceCompression() // this function will give strange compiler errors if a wrong type is specified.
	{
		size_t previousExpectedType = expectedType; // save for restoring in case of failure
		expectedType = TYPE(T);
		if (!isCompressable())
		{
			expectedType = previousExpectedType; // restore
			THROW NotCompressable();
		}
		forcecompression = true;
	}
	void forceCompression(TypeSpecifier t);
	bool isCompressable();
	bool isCompressed() const;

	//functions for setting type. T needs to be defualt constructable
	template<typename T>
	void makeType()
	{
		if (std::holds_alternative<T>(data)) return; // already right type
		data.emplace<T>();
	}
	void makeType(TypeSpecifier t, TypeSpecifier subtype = 0);
	void makeObject();
	void makeList();

	//TODO: how to access data
	template<typename T>
	bool is()
	{
		return std::holds_alternative<T>(data);
	}

	template<typename T>
	T& get()
	{
		return std::get<T>(data);
	}

	template<typename T>
	const T& get() const
	{
		return std::get<T>(data);
	}

	template<typename T>
	T* get_if()
	{
		return std::get_if<T>(&data);
	}

	template<typename T>
	void set(const T& value)
	{
		data = value;
	}

	template<typename T>
	void set(T value)
	{
		data = value;
	}

	template<typename T>
	void push_back(T value)
	{
		if (!isList())
			makeList();
		std::vector<OOD>& vec = std::get<std::vector<OOD>>(data);
		vec.push_back(value);
	}

	template<typename T>
	void insert(std::string key, T value)
	{
		if (!isObject())
			makeObject();
		std::unordered_map<std::string, OOD>& map = std::get<std::unordered_map<std::string, OOD>>(data);
		map.insert(std::make_pair(key, value));
	}

	void resize(size_t newSize);
	OOD& insert(std::string key); // if the key is already existing, it returns the existing value. Just like operator[]
	OOD& insert(); // adds a new element to the list and returns a reference to it
	void erase(std::vector<size_t> indieces);
	void erase(size_t index);
	void erase(std::string key);

	std::vector<OOD>* getListAccess();
	std::unordered_map<std::string, OOD>* getMapAccess();

	//object and array functions
	//when using operator[], the object will be turned into an list or object
	OOD& operator[](size_t index);
	OOD& operator[](std::string key); // forwards to operator[](const char*)
	OOD& operator[](const char* key); // like std::string key


	//implicit conversion operators
	template<typename T>
	operator T()
	{
		T* ptr = std::get_if<T>(&data);
		if (ptr)
			return *ptr;

		//The second argument is just to make the type of T printable
		THROW BadImplicitConversion(data.index(), TYPE(T));
	}

	//now make like sixtynine constructos, because i dont know if you can use templates in a union
	OOD();
	template<typename T>
	OOD(T value) : data(value), forcecompression(false), forcetype(false), expectedType(0), forcedType(0), objectarray(false), enable_string_obfuscation(enable_string_obfuscation_static) {}

	template<typename T>
	OOD& operator= (const T& value)
	{
		enable_string_obfuscation = enable_string_obfuscation_static;
		if ((const void*)this == (const void*)&value)
			return *this;

		if (false)
			if (TYPE(T) != forcedType)
			{
				THROW BadConstructionType(forcedType, TYPE(T));
			}

		data = value;
		return *this;
	}

	friend std::ostream& operator<< (std::ostream& out, OOD& ood);
	friend bool operator== (const OOD& ood1, const OOD& ood2);
};

DF_API std::ostream& operator<< (std::ostream& out, std::wstring wstr);