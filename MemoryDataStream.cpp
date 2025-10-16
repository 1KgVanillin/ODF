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

void MemoryDataStream::setPreprocessor(std::function<void(char*, size_t)> preprocessorFunction)
{
	preprocessor = preprocessorFunction;
	enablePreprocessing = true;
}

void MemoryDataStream::disablePreprocessor()
{
	enablePreprocessing = false;
}

void MemoryDataStream::setPostprocessor(std::function<void(char*, size_t)> postprocessorFunction)
{
	postprocessor = postprocessorFunction;
	enablePostprocessing = true;
}

void MemoryDataStream::disablePostprocessor()
{
	enablePostprocessing = false;
}

void MemoryDataStream::deallocateDataOnDestruction(bool deallocate)
{
	deallocateOnDestruction = deallocate;
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

	}
	else
	{
		delete[] start;
		selfAllocated = false;
		start = current = nullptr;
	}	
}

MemoryDataStream MemoryDataStream::move()
{
	MemoryDataStream other = *this; // copy everything

	selfAllocated = false;
	deallocateOnDestruction = false;
	start = current = nullptr;
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
	disablePreprocessor();
	disablePostprocessor();
	deallocateDataOnDestruction(false);
	selfAllocated = false;
	finished = false;
	dynamicAllocationMeta = nullptr;
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

void MemoryDataStream::DynamicAllocationMetadata::newChunk()
{
	chunks.push_back(new char[chunkSize]);
	currentPosition = 0;
}

void MemoryDataStream::write(const char* bytes, size_t size)
{
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
		THROW WriteViolation("MemoryDataStream exceeded specified range.");
	}
}

void MemoryDataStream::DynamicAllocationMetadata::write(char* data, size_t size)
{
	if (!chunks.size())
		newChunk();

	
	memcpy(chunks[chunks.size() - 1], data, size)
}

void MemoryDataStream::DynamicAllocationMetadata::read(size_t startindex, char* dest, size_t size)
{
	size_t chunk = startindex / chunkSize;
	size_t endChunk = (startindex + size) / chunkSize;
	unsigned char chunkPosition = startindex % chunkSize;
	unsigned char endChunkPosition = (startindex + size) % chunkSize;

	// copy start chunk
	memcpy(dest, chunks[chunk], chunkSize - chunkPosition);
	if (chunkSize - chunkPosition < size || size < chunkSize)
		return;

	// copy middle (full) chunks
	for (size_t i = chunk + 1; i <= endChunk - 1; i++)
		memcpy(dest + (chunkSize - chunkPosition), chunks[i], chunkSize);

	if (endChunkPosition) // check this somehow
	// copy last chunk
	memcpy((dest + size) - (chunkSize - endChunkPosition), chunks[endChunk], endChunkPosition + 1);
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

unsigned char MemoryDataStream::peek() const
{
	if (current < firstInvalidAddress || insecure)
		return *current;
}

char MemoryDataStream::read()
{
	if (current < firstInvalidAddress || insecure)
		return *current++;
	else
	{
		THROW std::out_of_range("MemoryDataStream exceeded specified range.");
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
		THROW std::out_of_range("MemoryDataStream exceeded specified range.");
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
		THROW std::out_of_range("MemoryDataStream exceeded specified range.");
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
		THROW std::out_of_range("MemoryDataStream exceeded specified range.");
	}
}

char* MemoryDataStream::data()
{
	return start;
}

size_t MemoryDataStream::size()
{
	return firstInvalidAddress - start;
}

size_t MemoryDataStream::sizeLeft()
{
	return firstInvalidAddress - current;
}

MemoryDataStream::MemoryDataStream(size_t size)
{
	allocate(size);
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

MemoryDataStream::WriteViolation::WriteViolation(std::string message) : runtime_error(message) {}

MemoryDataStream::IncompleteWrite::IncompleteWrite(std::string message) : runtime_error(message) {}

MemoryDataStream::DynamicAllocationMetadata::DynamicAllocationMetadata()
{
	chunkSize = defaultChunkSize;
	currentPosition = 0;
}

void MemoryDataStream::DynamicAllocationMetadata::deallocate()
{
	for (void* chunk : chunks)
		delete chunk;
	chunks.clear();
	currentPosition = 0;
}

MemoryDataStream::DynamicAllocationMetadata::~DynamicAllocationMetadata()
{
	deallocate();
}

void MemoryDataStream::DynamicAllocationMetadata::write(size_t size, const char* data)
{
	vecdata.reserve(vecdata.size() + size);
	for (size_t i = 0; i < size; i++)
		vecdata.push_back(data[i]);
}

size_t MemoryDataStream::DynamicAllocationMetadata::size() const
{
	return vecdata.size();
}

const char* MemoryDataStream::DynamicAllocationMetadata::data() const
{
	return vecdata.data();
}