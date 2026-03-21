# MemoryDataStream-Class

## Summary

The MemoryDataStream (MDS for short) offers a stream-like interface for writing and reading from memory.    
It is only made for writing **or** reading but can theoretically handle both at the same time, though this might lead to undefined data.

Every MDS-Object can operate in 3 different Modes:

- Default (Write): The MDS can be used with functions like `write` and all generic functions.
- ReadOnly: The MDS only allows `read` access and read-only generic functions.
- Dynamic (WriteOnly): must be explicitely activated with `enableDynamicAllocation`, allows dynamic allocation of its own data, but is write only. For read access, use `getReadAccess`

## Error Handling

Error are handled through custom exceptions.   
All Exceptions are std compatible and inherit from `std::runtime_error`

## Construction

### Default Write Mode

Construct an MDS that allocates its own data:
```cpp
MemoryDataStream mem(datasize);
```

How to write to an existing array:
```cpp
const size_t datasize = 32;
char* data = new char[datasize];

// no overflow protection
MemoryDataStream mem(data);

// throws MemoryDataStream::WriteViolation on overflow
MemoryDataStream mem(data, datasize);

// using the first address, the MDS must NOT write to
char* firstInvalidAddress = data + datasize;
MemoryDataStream mem(data, firstInvalidAddress);
```
Using the last construction method from above, the MemoryDataStream can be constructed onto a vector:
```cpp
std::vector<char> data;
// fill vector

// get addresses from iterators
// .data() and .size() can also be used.
MemoryDataStream(&(*data.begin()), &(*data.end()));
```

### ReadOnly Mode

The MDS Object is automatically configured to be read-only, if the pointer it receives on construction is const:
```cpp
const size_t datasize = 8;
const char* data = "somedata";

// no overflow protection
MemoryDataStream mem(data);

// throws MemoryDataStream::WriteViolation on overflow
MemoryDataStream mem(data, datasize);
```
This also works for the firstInvalidAddress like above:
```cpp
char* firstInvalidAddress = data + datasize;
MemoryDataStream(data, firstInvalidAddress);
```

### Dynamic Allocation Mode

An default Construction sets the MDS to Dynamic mode:
```cpp
MemoryDataStream mem;
// or
MemoryDataStream mem(0);
```

### Empty Construction

Some cases need the MDS object to be constructed without data.
This is done with either
```cpp
MemoryDataStream mem(nullptr);
```
or
```cpp
MemoryDataStream mem((const char*)nullptr);
```

The MDS can then be configured later:

Setting data:
```cpp
mem.setDataPtr(data); // also works with const data, just like the constructor
```

Specifying range:
```cpp
mem.setMaxSize(datasize);
// or
mem.setFirstInvalidAddress(data + datasize);
// or
mem.setLastValidAddress(data + datasize - 1);

// enable Security checks
mem.enableSecurity();
```

Switch to Dynamic Mode:
```cpp
mem.enableDynamicAllocation();
```
Note that when switching to dynamic mode, the MDS allocates its own data on the heap and copies everything
that was writting to it until then to it, so this might be slow on large datasets if it is not called relatively fast after construction

## Other Functions (Default Mode)

### void setByteEncryptor(std::function<void(char&)> byteEncryptor)

uses the provided function to encrypt every Byte before it is written to the target memory.
Enables Byte encryption.

### void disableByteEncryption()

Disables Byte encryption.

### void setPostProcessor(std::function<void(char*, size_t)> postprocessorFunction)

enable PostProcessing. The Post processor is executed on the target data when the MDS goes out of scope or finish() is called.

### void disablePostProcessing()

Disables PostProcessing

### void deallocateDataOnDestruction(bool deallocate)

If true, delete the target data when the MDS goes out of scope.

### void enableDynamicAllocation()

Switch to Dynamic Mode.

### void allocate(size_t size)

If the MDS already owns data, it gets deallocated. Then Allocate new data with size `size` and use it as new target data. Automatically deleted when MDS goes out of scope, `finish()` is called or `deallocate()` is called.

### void deallocate()

Deallocates previously dynamically allocated data explicitely.
Does nothing if the MDS does not own the target data.

## Other Functions (ReadOnly Mode)

### void setByteDecryptor(std::function<void(char&)> byteDecryptor)

Decrypts Every Byte after it is read from the target memory.

### void disableByteEncryption()

Disable Byte encryption after it was enabled with `setByteDecryptor()` or `setByteEcryptor()`

### void setPreprocessor(std::function<void(char*, size_t)> preprocessorFunction, bool instantExecute = true)

Specifies a preprocessor. The preprocessor processes the target data before it is read.
If `instantExecute` is True, it immediatly processes over the data, other wise it is executed when `executePreprocessor()` is called.

### executePreprocessor()

Executes the Preprocessor

### void disablePreprocessing()

Disables the preprocessor. A following call to `executePreprocessor()` will be ignored.

## Other Functions (Dynamic Mode)



## Generic Functions

Those functions can be called in any mode. Functions with [nodynamic] cannot be called in dynamic mode though.

### void enableSecurity()

enables the overflow protection. Use after specifying the data range after construction

### void enableStringObfuscation(bool obfuscation = true)

Make all String written to the target memory unreadable with basic obfuscation.   
***This does not replace any form of encryption***

### void disableStringObfuscation()

Disables String Obfuscation. `enableStringObfuscation(false)` can also be used.

### void makeReadOnly()

explicitely makes the MDS Object read-only

### void finish()

Destroys the object. Is called by the destructor.   
Using the MDS Object after a call to `finish()` leads to undefined behaiviour.

### MemoryDataStream move()

Moves the data ownership to another MDS.

### void move(MemoryDataStream& mem)

Moves the data ownership to mem.

## Memory Functions

The actual read and write functions, used to interact with the target memory.

Write functions:
```cpp
// write single value
mem.write(object); // templated, works with any type, also custom types
// write char*
mem.write(data, datasize);
```

Read functions:
```cpp
// read single byte
char byte = mem.read();
// read to char*
mem.read(destination, size);
// manually read size bytes. It is save to read at least size bytes from ptr.
// ptr can be used to access to whole target data, though this can lead
// to undefined behaiviour.
char* ptr = mem.read(size);

// read custom type (makes temporary copy of type and returns it)
int i = mem.read<int>();
```

Peek Functions (read the next data without increasing the read position):
```cpp
// peek byte
char byte = mem.peek();
// peek to destination
mem.peek(destination, size);
// peek custom type
int i = mem.peek<int>();
```

Accessing the previously read or written byte (used in SizeSpecififierSpecifier)
```cpp
// read previous byte
char byte = mem.peekPrevious();

// set previous byte
mem.setPrevious(5ui8);
```

Size:
```cpp
// get size
size_t size = mem.size();

// get size left until end
size_t sizeleft = mem.sizeLeft();
```

Get raw data access:
```cpp
char* datapointer = mem.data();
```

