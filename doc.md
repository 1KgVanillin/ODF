# Documentation

## File structure
 
Object will be saved like shown below.
The file just starts with the first object.
When this first Object ends, the file ends.

Empty Arrays or objects could cause bugs as they are somtimes not implemented

## Supported Types:

- Objects
- Arrays
- Signed Int64 (Long)
- Unsigned Int64 (Long)
- Signed Int32 (Int)
- Unsigned Int32 (Int)
- Signed Int16 (Short)
- Unsigned Int16 (Short)
- Signed Byte (Char)
- Unsigned Byte (Char)
- Float
- Double
- String
- WString

## General Format
- .ood (Optimized Object Data Format)
- .oodr (Optimized Object Data Notation (Human Readable))

1 Byte: Typespecifier  
n Bytes: data

## Typespecifiers and Type

|Type|SpecifierName|Specifier|Size|Description|
|---|---|---|---|
|NULL|NULL|0x00|1|NULL, String terminator|
|Byte|BYTE|0x01|1|Signed Int8|
|UByte|UBYTE|0x02|1|Unsigned Int8|
|Short|SHORT|0x03|2|Signed Int16|
|UShort|USHORT|0x04|2|Unsigned Int16|
|Int|INT|0x05|4|Signed Int32|
|UInt|UINT|0x06|4|Unsigned Int32|
|Long|LONG|0x07|8|Signed Int64|
|ULong|ULONG|0x08|8|Unsigned Int64|
|Float|FLOAT|0x09|4|Floating Point|
|Double|DOUBLE|0x0A|8|Double Precision Floating Point|
|String|CSTR|0x0B|>=2|Null terminated CString|
|WideString|WSTR|0x0C|>=3|Null terminated WString|
|MixedList|MLIST|0x0D|n|Array of different types|
|MixedObject|MOBJ|0x0E|n|Object with only one value type|
|Object|OBJ|0x0F|n|Object with key-value pairs|
|List|LIST|0x010|n|Array of type|
|ObjectArray|OBJLIST|0x11|n|For objects optimized Array|
|LargeArray|LLIST|0x12|n|List with 2 bytes size specifier (max 65k)|
|MaxArray|MXLIST|0x13|n|List with 4 bytes size specifier (max 4G)|
|END|END|0xFF|1|Object and Array terminator|
|

In the mixed array, no size is needed, as the END terminator can be specified instead of a type

### Object Format Example

__json text__
```
{
	"number1": 100,
	"number2": 300
}
```
__Raw__  
MOBJ // specifier  
USHORT // type   
number1 NULL // key1  
number2 NULL // key2  
NULL // terminator. (empty key)


### Mixed Object Format Example

__Json text__
```
{
	"AGE": 69,
	"NAME": "FRANZ"
}
```

__Raw__  
0x0D // object specifier  
A  
G  
E // key  
NULL // seperator  
UByte // valuetype  
69 // value  
N  
A  
M  
E // key2  
NULL // seperator  
CSTR // value type  
F  
R  
A  
N  
Z // Data  
NULL   // an empty key at the end terminates the object

__OODR Notation__  
```
OBJECT
	Byte AGE 69
	CSTR NAME "FRANZ"
END

```

### Array Format Example

__Important: Non mixed arrays must not contain Objects__
For same type objects, a object array must be used and for mixed objects a Mixed List

__json text__  
```
[
	1,
	2,
	3
]
```

__Raw__  
0x0D // array  
0x01 // data type  
3 // uint8 size  
0x69  
0x11  
0x42 // data

__OODR Notation__  
```
LIST BYTE
	0x69
	0x11
	0x42
END
```


## Mixed Array Format Example

__json text__
```
[
	1,
	"fisch",
	4.5
]
```

__Raw__  
MLIST // mixed array  
BYTE // data type  
1 // data  
CSTR // data type  
F  
I  
S  
C  
H  
NULL // data  
FLOAT // data type  
4.5 // data  
END // Array terminator

__OODR Notation__
```
MLIST
	BYTE 1
	CSTR "FISCH"
	FLOAT 4.5
END
```

## Object Array Format Example

_Note: all objects must be the same_

__json text__
```
[
	{"name":"juergen","age":22},
	{"name":"peter", "age": 40}
]
```

__Raw__  
OBJLIST // object array  
size // depending on exact type, like in the nonmixed array  
n  
a  
m  
e  
NULL // key  
CSTR // value type  
a  
g  
e  
NULL // key  
BYTE // value  
NULL // seperator  
juergen  
22 // data  
peter  
NULL
40 // data  

__OODR Notation__
```
OBJLIST
	CSTR name
	BYTE age
NULL
	juergen 22
	peter 40
```
