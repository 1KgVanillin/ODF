#pragma once
#include <variant>
#include <memory>
#include <vector>

namespace oi
{
	using Byte = unsigned char;

	// modifiers
	constexpr Byte EXTRA = 0x20;

	// types
	constexpr Byte END = 0x0;
	constexpr Byte UNDEF = 0x0 | EXTRA;
	constexpr Byte BYTE = 0x1;
	constexpr Byte UBYTE = BYTE | EXTRA;
	constexpr Byte SHORT = 0x2;
	constexpr Byte USHORT = SHORT | EXTRA;
	constexpr Byte INT = 0x3;
	constexpr Byte UINT = INT | EXTRA;
	constexpr Byte LONG = 0x4;
	constexpr Byte ULONG = LONG | EXTRA;
	constexpr Byte FLOAT = 0x5;
	constexpr Byte DOUBLE = FLOAT | EXTRA;
	constexpr Byte CSTR = 0x6;
	constexpr Byte WSTR = CSTR | EXTRA;
	constexpr Byte FXOBJ = 0x7;
	constexpr Byte MXOBJ = FXOBJ | EXTRA;
	constexpr Byte FXLST = 0x8;
	constexpr Byte MXLST = FXLST | EXTRA;
	constexpr Byte DEFTYPE = 0x9;
	constexpr Byte USETYPE = DEFTYPE | EXTRA;
	constexpr Byte EXPORT = 0xA;
	constexpr Byte EXPORTAS = EXPORT | EXTRA;
	constexpr Byte IMPORT = 0xB;
	constexpr Byte IMPORTAS = IMPORT | EXTRA;

	// secondary size specifiers
	constexpr Byte S0 = 0;
	constexpr Byte S1 = 0x40;
	constexpr Byte S2 = 0x80;
	constexpr Byte S3 = 0xC0;

	// helpers
	template<size_t idx>
	constexpr Byte BuiltInType()
	{
		// the built in types are distributed to 0xnm
		// where n is a hexadecimal digit except 0 or 2 and m is a hexadecimal digit without constraints.
		// That gives (16 - 2) * 16 = 224 combinatiosn

		static_assert(idx < 224);
		if (idx <= 0xF)
			return idx | 0x10;
		return (idx - 0x10) + 0x20;
	}

	template<typename T>
	constexpr std::vector<Byte> value(const T& t)
	{
		std::vector<Byte> data(sizeof(T));
		memcpy(data.data(), reinterpret_cast<const void*>(&t), sizeof(T));
		return std::move(data);
	}

	using OIAny = std::variant<Byte, std::vector<Byte>>;
	using RawODF = std::initializer_list<OIAny>;

	std::tuple<std::shared_ptr<char[]>, size_t> CreateRawData(const RawODF& rodf);
	

	std::vector<Byte> operator ""strODF(const char* str, size_t len);
}