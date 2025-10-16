#pragma once
#include <string>
#include <functional>
#include <stdexcept>
#include <iostream>
#include "Definitions.h"

// lets make a safe pointer quicky. Throws std::out_of_range if invalid access
// This class is a smart pointer, safe memory accessor, memory stream and encryption tool in just one class :)
class DF_API MemoryDataStream
{
	class DynamicAllocationMetadata // for allocating memory, write only.
	{
		std::vector<char> vecdata;
	public:
		template<typename T>
		void write(T data)
		{
			write(sizeof(T), (char*)&data);
		}
		void write(size_t size, const char* data);
		size_t size() const;
		const char* data() const;
	};

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
	// bool dynamicAllocation; // this is represented by a nullptr or valid address in dynamicAllocationMeta*
	DynamicAllocationMetadata* dynamicAllocationMeta;
	void migrateData();

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
	void disablePreprocessor();
	void setPostprocessor(std::function<void(char*, size_t)> postprocessorFunction); // key needs to be either hardcoded or passed via lampda capture
	void disablePostprocessor();
	void deallocateDataOnDestruction(bool deallocate);
	void enableStringObfuscation(bool obfuscation = true);
	void disableStringObfuscation();
	void enableDynamicAllocation(); // cannot be disabled, stream can be used as before. Uses a std::vector internally. Performance heavy enabled when data is already present, as it is migrated to a std::vector
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

	MemoryDataStream(size_t size); // allocates own data, if 0 specified, dynamic allocation on write() calls.
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
	struct WriteViolation : public std::runtime_error
	{
		WriteViolation(std::string message);
	};
	// thrown if the stream hasnt reached its end on finish() if writeFullStream is specified
	struct IncompleteWrite : public std::runtime_error
	{
		IncompleteWrite(std::string message);
	};
};
