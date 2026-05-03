#include "GenericDebug.h"
#include "MemoryDataStream.h"

std::ostream& operator<<(std::ostream& out, std::wstring wstr)
{
	out << "L\"";
	for (wchar_t w : wstr)
		out << (char)w;
	out << "\"";
	return out;
}

void MemoryDataStream::setDataPtr(char* data)
{
	finish(); // try to deallocate owned data, incase setDataPtr was called after allocation
	baseinit();
	start = current = data;
	readOnly = false;
}

void MemoryDataStream::setDataPtr(const char* data)
{
	finish();
	baseinit();
	start = current = (char*)data;
	readOnly = true;
}


void MemoryDataStream::setMaxSize(size_t size)
{
	firstInvalidAddress = current + size;
}

void MemoryDataStream::setLastValidAddress(const char* lastValidAddr)
{
	firstInvalidAddress = (char*)lastValidAddr + 1;
}

void MemoryDataStream::setFirstInvalidAddress(const char* firstInvalidAddr)
{
	firstInvalidAddress = (char*)firstInvalidAddr;
}

void MemoryDataStream::enableSecurity(bool on)
{
	insecure = !on;
}

void MemoryDataStream::makeReadOnly()
{
	readOnly = true;
}

void MemoryDataStream::setByteEncryptor(std::function<void(char&)> byteEncryptor)
{
	encryptor = byteEncryptor;
	enableBytewiseEncryption = true;
}

void MemoryDataStream::setByteDecryptor(std::function<void(char&)> byteDecryptor)
{
	decryptor = byteDecryptor;
	enableBytewiseEncryption = true;
}

void MemoryDataStream::disableByteEncryption()
{
	encryptor = nullptr;
	decryptor = nullptr;
	enableBytewiseEncryption = false;
}

void MemoryDataStream::setPreprocessor(std::function<void(char*, size_t)> preprocessorFunction, bool instantExecute)
{
	preprocessor = preprocessorFunction;
	enablePreprocessing = true;
}

void MemoryDataStream::executePreprocessor()
{
	if (enablePreprocessing)
		preprocessor(start, firstInvalidAddress - start);
}

void MemoryDataStream::disablePreprocessing()
{
	enablePreprocessing = false;
}

void MemoryDataStream::setPostprocessor(std::function<void(char*, size_t)> postprocessorFunction)
{
	postprocessor = postprocessorFunction;
	enablePostprocessing = true;
}

void MemoryDataStream::disablePostprocessing()
{
	enablePostprocessing = false;
}

void MemoryDataStream::deallocateDataOnDestruction(bool deallocate)
{
	deallocateOnDestruction = deallocate;
}

void MemoryDataStream::enableStringObfuscation(bool obfuscation)
{
	obfuscated = obfuscation;
}

void MemoryDataStream::disableStringObfuscation()
{
	obfuscated = false;
}

void MemoryDataStream::enableDynamicAllocation()
{
	if (dynamicAllocation)
		delete dynamicAllocation;
	dynamicAllocation = new DynamicAllocationMetadata;
	dynamicAllocation->vecdata.reserve(1); // reserve 1 byte, so vecdata is forced to get an address
	start = dynamicAllocation->vecdata.data();
}

void MemoryDataStream::allocate(size_t size)
{
	if (selfAllocated)
		delete[] start;

	deallocateDataOnDestruction(true);
	selfAllocated = true;

	start = current = new char[size];
	firstInvalidAddress = start + size;

	baseinit();
}

void MemoryDataStream::deallocate()
{
	if (dynamicAllocation)
	{
		delete dynamicAllocation;
		dynamicAllocation = nullptr;
	}
	else
	{
		delete[] start;
		selfAllocated = false;
		start = current = nullptr;
	}	
}

MemoryDataStream& MemoryDataStream::reset()
{
	setCursor(0);
	return *this;
}

void MemoryDataStream::setCursor(size_t index)
{
	current = start + index;
}

MemoryDataStream MemoryDataStream::move()
{
	MemoryDataStream other = *this; // copy everything

	selfAllocated = false;
	deallocateOnDestruction = false;
	start = current = nullptr;
	dynamicAllocation = nullptr;
	baseinit();
	return other;
}

void MemoryDataStream::move(MemoryDataStream& mem)
{
	mem = *this; // copy everything

	selfAllocated = false;
	deallocateOnDestruction = false;
	start = current = nullptr;
	baseinit();
}

void MemoryDataStream::finish()
{
	// handle write only streams
	if (dynamicAllocation)
	{
		finished = true;
		if (enablePostprocessing)
			postprocessor(dynamicAllocation->vecdata.data(), dynamicAllocation->size());
		delete dynamicAllocation;
		dynamicAllocation = nullptr;
		return;
	}

	finished = true;
	if (enablePostprocessing)
		postprocessor(start, firstInvalidAddress - start);

	// doesn't makes sense in compination with post processing, but if you have a read only stream, you dont have to deallocate manually
	if (deallocateOnDestruction)
		deallocate();

	if (writeFullStream && current != firstInvalidAddress)
	{
		THROW IncompleteWrite("A MemoryDataStream was deallocated before it was finished.");
	}
}

//this method doesnt mess with the raw data. The NULL terminator isn't included in the std::string. The function never generates zeros
void MemoryDataStream::makeReadable(std::string& str)
{
	//swap some stuff
	for (size_t i = 0; i < str.size() - 1; i += 2)
		std::swap(str[i], str[i + 1]);
	//invert bits
	for (char& c : str)
		if (c != ~0)
			c = ~c;
}

// TODO
void MemoryDataStream::migrateData()
{
}

void MemoryDataStream::makeUnreadable(std::string& str)
{
	//invert bits
	for (char& c : str)
		if (c != ~0)
			c = ~c;
	//swap some stuff
	for (size_t i = 0; i < str.size() - 1; i += 2)
		std::swap(str[i], str[i + 1]);
}

void MemoryDataStream::makeReadable(std::wstring& wstr)
{
	//swap some stuff
	for (size_t i = 0; i < wstr.size() - 1; i += 2)
		std::swap(wstr[i], wstr[i + 1]);
	//invert bits
	for (wchar_t& c : wstr)
		if (c != ~0)
			c = ~c;
}

void MemoryDataStream::baseinit()
{
	enableSecurity();
	disableByteEncryption();
	disablePreprocessing();
	disablePostprocessing();
	deallocateDataOnDestruction(false);
	selfAllocated = false;
	finished = false;
	dynamicAllocation = nullptr;
}

void MemoryDataStream::makeUnreadable(std::wstring& wstr)
{
	//invert bits
	for (wchar_t& c : wstr)
		if (c != ~0)
			c = ~c;
	//swap some stuff
	for (size_t i = 0; i < wstr.size() - 1; i += 2)
		std::swap(wstr[i], wstr[i + 1]);
}

void MemoryDataStream::write(const char* bytes, size_t size)
{
	if (dynamicAllocation)
	{
		dynamicAllocation->write(bytes, size);
		// reassign start as dynamicAllocation->write has propably modified the location of the data
		start = dynamicAllocation->vecdata.data();
		firstInvalidAddress = start + dynamicAllocation->size();
		return;
	}

	if (readOnly)
	{
		THROW ReadOnlyViolation("tried to write to read only location using MemoryDataStream.");
	}

	// validate pointer
	if (current + (size - 1) < firstInvalidAddress || insecure)
	{
		memcpy(current, bytes, size);
		current += size;
	}
	else
	{
		THROW AccessViolation("MemoryDataStream exceeded specified range. [write(const char*, size_t)]");
	}
}

void MemoryDataStream::DynamicAllocationMetadata::write(const char* data, size_t size)
{
	vecdata.reserve(vecdata.size() + size);
	for (size_t i = 0; i < size; i++)
		vecdata.emplace_back(data[i]);
}

void MemoryDataStream::DynamicAllocationMetadata::read(size_t startindex, char* dest, size_t size)
{
	if (size > vecdata.size() - startindex)
		throw AccessViolation("MemoryDataStream exceeded specified range (explicit read)");

	memcpy(dest, vecdata.data() + startindex, size);
}

std::string MemoryDataStream::readStr()
{
	std::string result;
	while (peek()) // while the next byte is not a NULL terminator
		result += read();
	// read last null terminator
	read();
	// deobfuscate
	if (obfuscated)
		makeReadable(result);
	return result;
}

std::wstring MemoryDataStream::readWstr()
{
	std::wstring result;
	while (peek<wchar_t>())
		result += read<wchar_t>();
	// read last null terminator
	read<wchar_t>();
	// deobfuscate
	if (obfuscated)
		makeReadable(result);
	return result;
}

void MemoryDataStream::writeStr(std::string str)
{
	// obfuscate
	if (obfuscated)
		makeUnreadable(str);
	write(str.c_str(), str.size() + 1); // 1000 IQ because reusing the null terminator
}

void MemoryDataStream::writeWstr(std::wstring wstr)
{
	// obfuscate
	if (obfuscated)
		makeUnreadable(wstr);
	write((char*)wstr.c_str(), (wstr.size() + 1) * sizeof(wchar_t)); // 1000 IQ because reusing the null terminator
}

void MemoryDataStream::skip(size_t numBytes)
{
	if (current + (numBytes - 1) < firstInvalidAddress || insecure)
		current += numBytes;
}

char MemoryDataStream::peek()
{
	if (current < firstInvalidAddress || insecure)
		return *current;
	throw AccessViolation("MemoryDataStream exceeded specified range. [peek()]");
}

char MemoryDataStream::read()
{
	if (current < firstInvalidAddress || insecure)
		return *current++;
	else
	{
		THROW AccessViolation("MemoryDataStream exceeded specified range. [read()]");
	}
}

void MemoryDataStream::peek(char* destination, size_t size)
{
	if (current + (size - 1) < firstInvalidAddress || insecure)
	{
		memcpy(destination, current, size);
	}
	else
	{
		THROW AccessViolation("MemoryDataStream exceeded specified range. [peek(char*, size_t)]");
	}
}

void MemoryDataStream::read(char* destination, size_t size)
{
	if (current + (size - 1) < firstInvalidAddress || insecure)
	{
		memcpy(destination, current, size);
		current += size;
	}
	else
	{
		THROW AccessViolation("MemoryDataStream exceeded specified range. [read(char*, size_t)]");
	}
}

char* MemoryDataStream::read(size_t size)
{
	if (current + (size - 1) < firstInvalidAddress || insecure)
	{
		char* currentBeforeIncrease = current;
		current += size;
		return currentBeforeIncrease;
	}
	else
	{
		THROW AccessViolation("MemoryDataStream exceeded specified range. [read(size_t)]");
	}
}

char* MemoryDataStream::data() const
{
	return start;
}

size_t MemoryDataStream::size() const
{
	return firstInvalidAddress - start;
}

size_t MemoryDataStream::sizeLeft() const
{
	return firstInvalidAddress - current;
}

size_t MemoryDataStream::pos() const
{
	if (dynamicAllocation)
		return dynamicAllocation->size();
	else
		return current - start;
}

char MemoryDataStream::peekPrevious() const
{
	if (current <= start)
	{
		THROW std::out_of_range("Out of range exception in MemoryDataStream::peekPrevious() : no provious byte available");
	}
	return *(current - 1);
}

void MemoryDataStream::setPrevious(char byte)
{
	if (current <= start)
	{
		THROW std::out_of_range("Out of range exception in MemoryDataStream::setPrevious() : no provious byte available");
	}
	*(current - 1) = byte;
}

bool MemoryDataStream::empty() const
{
	return !size();
}

bool MemoryDataStream::saveToFile(const std::string& file) const
{
	std::ofstream out(file, std::ios::binary);
	if (out.fail())
		return false;
	
	out.write(start, pos());
	out.close();
	return true;
}

MemoryDataStream::MemoryDataStream(size_t size)
{
	baseinit();
	if (size)
		allocate(size);
	else
	{
		// dynamic allocation
		enableDynamicAllocation();
	}
}

MemoryDataStream::MemoryDataStream(char* data)
{
	setDataPtr(data);
}

MemoryDataStream::MemoryDataStream(const char* data)
{
	setDataPtr(data);
}

MemoryDataStream::MemoryDataStream(char* data, char* firstInvalidAddr)
{
	setDataPtr(data);
	setFirstInvalidAddress(firstInvalidAddr);
}

MemoryDataStream::MemoryDataStream(const char* data, const char* firstInvalidAddr)
{
	setDataPtr(data);
	setFirstInvalidAddress(firstInvalidAddr);
}

MemoryDataStream::MemoryDataStream(char* data, size_t size)
{
	setDataPtr(data);
	setFirstInvalidAddress(data + size);
}

MemoryDataStream::MemoryDataStream(const char* data, size_t size)
{
	setDataPtr(data);
	setFirstInvalidAddress(data + size);
}

MemoryDataStream::~MemoryDataStream()
{
	if (!finished)
		finish();
}

MemoryDataStream::ReadOnlyViolation::ReadOnlyViolation(std::string message) : runtime_error(message) {}

MemoryDataStream::AccessViolation::AccessViolation(std::string message) : runtime_error(message) {}

MemoryDataStream::IncompleteWrite::IncompleteWrite(std::string message) : runtime_error(message) {}

MemoryDataStream::DynamicAllocationMetadata::DynamicAllocationMetadata()
{
}

MemoryDataStream::DynamicAllocationMetadata::~DynamicAllocationMetadata()
{
}

size_t MemoryDataStream::DynamicAllocationMetadata::size() const
{
	return vecdata.size();
}

const char* MemoryDataStream::DynamicAllocationMetadata::data() const
{
	return vecdata.data();
}

MemoryDataStream::WriteOnlyViolation::WriteOnlyViolation(std::string message) : runtime_error(message)
{
}
