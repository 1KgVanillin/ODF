# Using the C++ API

# Primitive Types

```cpp
// assignment
ODF number = 5;
ODF floatingPoint = 5.0;
ODF string = "hello";
ODF wstring = L"hello";

// basic arithmetic
ODF str = string + "test";
ODF copy = number; // copy by value
ODF result = number * 67; // available operators: +, -, *, /, =, ==, !=

// custom types are supported with templates
struct Custom { ... };
Custom operator+(const ODF& odf, const Custom& custom) { ... }
Custom custom = result + Custom(5);

// implicit conversion
int i = result;
```

# Loading and Saving

For more details on the `MemoryDataStream` class see [MemoryDataStream](MemoryDataStream.md).

## Loading

```cpp
// load from a file
odf.loadFromFile("data.odf");

// load from Memory
const char* data = new char[size];
...
odf.loadFromFile(data, size);

// or use a MemoryDataStream
MemoryDataStream mds(data, size);
odf.loadFromFile(mds); // this is actually what is done in ODF::loadFromFile(const char*, size_t)

// load from input stream (std::istream)
odf.loadFromStream(std::cin);
```

## Saving

```cpp
// save to a file
odf.saveToFile("data.odf");

// save to Memory
char* data = new char[size];
odf.saveToMemory(data, size);

// if the actual size is needed
size_t bytesWritten;
odf.saveToMemory(data, size, &bytesWritten);

// or use the MemoryDataStream
MemoryDataStream mds; // default construction = dynamic data allocation
odf.saveToMemory(mds);

// save to output stream (std::ostream)
odf.saveToStream(std::cout);
```

# Complex Types

Complex types which contains child elements, like lists or objects.

## Mixing

Every complex type exists in a __fixed__ and a __mixed__ version.  
With __fixed__ complex types, all child element have the same type.  
With __mixde__ complex types, this is not neccessary.  

How to find out wether and ODF object is fixed or mixed at runtime:
```cpp
ODF odf = { 5, 6 }; // fixed, because contains only 1 type (int)

bool fixed = odf.type.isFixed(); // true
bool mixed = odf.type.isMixed(); // false
fixed = odf.type.byte() | 0b0010'0000; // also possible
```

## Iterating

The `ODF::List` and `ODF::Object` subclass define their own iterators.  

Additionally `ODF::for_each(Functor&& func)` can be used like std::for_each.

## Lists

Lists are used like `std::vector`, though they don't support all functions yet.  

Supported functions currently are:
- void push_back(const ODF& element)
- void push_front(const ODF& element)
- void insert(size_t where, ODF& element)
- void erase(size_t index)
- void erase(size_t where, size_t numberOfElements)
- size_t find(const ODF& odf)
- ODF& at(size_t index) (+ const overload)
- ODF& operator[](size_t index) (+ const overload)
- size_t size() const
- void clear()
- a noexcept version of every function (`_nothrow`-suffix)
- range-based for loops: `for (auto& it : odf.list())

To create a list `void ODF::makeList()` should be used.  
To create fixed lists the overload `void ODF::makeList(const ODF::Type&)` or `void ODF::makeList(const ODF::TypeSpecifier&)` can be used.  
Additionally `push_back`, `push_front` and `insert` will implicitely convert the ODF object to a __mixed__ list, if called on a non-list typed ODF object.

## Objects

Objects are used like `std::unordered_map`, though they don't support all functions yet.

Supported functions currently are (Pair = ODF::Object::Pair)
- void insert(const std::string& key, const ODF& odf)
- void insert(const Pair& value)
- void insert(const std::map<std::string, ODF>& map)
- void insert(const std::unordered_map<std::string, ODF>& umap)
- void insert(const std::vector<Pair>& pairs)
- void insert(const std::initializer_list<Pair>& pairs)
- void erase(const std::string& key)
- bool contains(const std::string& key) const
- size_t count(const std::string& key) const
- void clear()
- size_t size() const
- ODF& at(const std::string& key) (+ const and const char* versions)
- ODF& operator[](const std::string& key) (+ const and const char* versions)
- range-based for loops: `for (auto& it : odf.object())`

To create an object `void ODF::makeObject()` should be used.  
To create a mixed object `void ODF::makeObject(const ODF&)` or `void ODF::makeObject(const ODF::TypeSpecifier&)`.
Additionally, every `insert` function will implicitely convert the ODF object into a __mixed__ object.

## Aggregate initialization of complex types

### Lists

```cpp
// just like you'd expect:
ODF list = { 5, 6, 7 }; // automatically fixed of possible
ODF mlist = { 5, "hello" }; // mixed
```

### Objects

just dont. :flushed:


## Utilities

### ODF& makeImmutable()

makes the type immutable and returns the current object.

### bool isConvertableTo(const Type& other) const

checks wether the current object can be converted to `other`.

### void convertTo(const Type& newType)

attempts a conversion to `newType`

### bool covertToIfConvertable(const Type& newType)

attempts a convertion to `newType` if `isConvertableTo(newType)` is true.

### ODF getAs(const Type& newType) const

returns the current object as another type by __value__.

### template\<typename T\> T& get()

forwards the owned std::variant<...> to std\:\:get\<T\> and returns it

### simple access functions

- const char* c_str() const;
- const wchar_t* w_str() const;
- size_t number() const;
- float float32() const;
- double float64() const;
- ODF::Object& object();
- ODF::List& list();