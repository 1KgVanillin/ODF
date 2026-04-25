#include "ODFUtil.hpp"

/*
* examples:
* IMPORT 67 AS 69
* IMPORT 69
* DEFTYPE BUILT-IN 67 AS CSTR
* DEFTYPE_1 67 AS FXOBJ<CSTR> {
*	"KEY",
*	"KEY"
* }
* 
*/

std::string to_utf8(const std::wstring& wstr) {
	if (wstr.empty()) return {};

	// 1. Convert wstring to a filesystem path
	// 2. Extract as a u8string (C++20 returns std::u8string)
	std::u8string u8str = std::filesystem::path(wstr).u8string();

	// 3. Convert std::u8string (char8_t) to std::string (char)
	// In C++20, char8_t and char are distinct types.
	return std::string(u8str.begin(), u8str.end());
}

// conversion utility
template<typename T>
union Converter { char bytes[sizeof(T)]; T data; };

void odfutil::DocumentDebugPrinter::makeNewline()
{
	newline.resize(indent + 1);
	newline[0] = '\n';
	memset(newline.data() + 1, '\t', indent);
}

void odfutil::DocumentDebugPrinter::addIndent()
{
	indent++;
	makeNewline();
}

void odfutil::DocumentDebugPrinter::removeIndent()
{
	if (!indent)
		return;
	indent--;
	makeNewline();
}

bool odfutil::DocumentDebugPrinter::isPrimitive(ODF::TypeSpecifier tspec)
{
	return tspec.smallType() <= 6;
}

bool odfutil::DocumentDebugPrinter::isVirtual(ODF::TypeSpecifier tspec)
{
	return tspec.smallType() >= 9;
}

void odfutil::DocumentDebugPrinter::printVirtual(ODF::TypeSpecifier tspec)
{
	using TS = ODF::TypeSpecifier;
	TS::Type t = tspec.withoutSSS();

	if (t == TS::DEFTYPE)
	{
		oss << Color::HardValue << std::hex;
		switch (tspec.sizeSpec())
		{
		case 0:
			oss << Color::Special << " BUILT-IN " << Color::HardValue << (unsigned int)document.read<unsigned char>();
			break;
		case 1:
			oss << (unsigned int)document.read<unsigned char>() << Color::Suffix << "_8";
			break;
		case 2:
			oss << document.read<unsigned short>() << Color::Suffix << "_16";
			break;
		case 3:
			oss << document.read<unsigned int>() << Color::Suffix << "_32";
			break;
		}
		oss << Color::Keyword << " AS ";
		if (isPrimitive(document.peek()))
			printTypeSpec(document.read(), false);
		else
			printComplexType();
	}
	else if (t == TS::USETYPE)
	{
		oss << Color::HardValue << std::hex;
		unsigned int realType = 0;
		switch (tspec.sizeSpec())
		{
		case 0: // doesn't really makes sense, as in this case USETYPE can be omitted
			realType = document.read<unsigned char>();
			oss << realType << Color::Warning << "(sss=0, 'USETYPE' should be omitted) ";
			break;
		case 1:
			realType = document.read<unsigned char>();
			oss << realType;
			break;
		case 2:
			realType = document.read<unsigned short>();
			oss << realType;
			break;
		case 3:
			realType = document.read<unsigned int>();
			oss << realType;
			break;
		}
		printTypeHint(realType, tspec.sizeSpec());
	}
}

void odfutil::DocumentDebugPrinter::printTypeSpec(ODF::TypeSpecifier tspec, bool asKeyword)
{
	if (asKeyword)
		oss << Color::Keyword;
	else
		oss << Color::TypeSpec;

	using TS = ODF::TypeSpecifier;
	switch (tspec.withoutSSS())
	{
	case TS::NULLTYPE: oss << "NULLTYPE"; break;
	case TS::UNDEFINED: oss << "UNDEFINED"; break;
	case TS::BYTE: oss << "BYTE"; break;
	case TS::UBYTE: oss << "UBYTE"; break;
	case TS::SHORT: oss << "SHORT"; break;
	case TS::USHORT: oss << "USHORT"; break;
	case TS::INT: oss << "INT"; break;
	case TS::UINT: oss << "UINT"; break;
	case TS::LONG: oss << "LONG"; break;
	case TS::ULONG: oss << "ULONG"; break;
	case TS::FLOAT: oss << "FLOAT"; break;
	case TS::DOUBLE: oss << "DOUBLE"; break;
	case TS::CSTR: oss << "CSTR"; break;
	case TS::WSTR: oss << "WSTR"; break;
	case TS::FXOBJ: oss << "FXOBJ"; break;
	case TS::MXOBJ: oss << "MXOBJ"; break;
	case TS::FXLIST: oss << "FXLIST"; break;
	case TS::MXLIST: oss << "MXLIST"; break;
	case TS::DEFTYPE: oss << "DEFTYPE"; break;
	case TS::USETYPE: oss << "USETYPE"; break;
	case TS::EXPORT: oss << "EXPORT"; break;
	case TS::EXPORTAS: oss << "EXPORT"; break;
	case TS::IMPORT: oss << "IMPORT"; break;
	case TS::IMPORTAS: oss << "IMPORT"; break;
	default: oss << "TYPE ERROR"; break;
	}
}

void odfutil::DocumentDebugPrinter::printComplexType()
{
	using TS = ODF::TypeSpecifier;
	TS t = document.read<unsigned char>();

	printTypeSpec(t.byte(), false);

	if (t == TS::FXOBJ)
	{
		oss << '<'; // fixtype.... quitted here. TODO
	}
}

// TODO
void odfutil::DocumentDebugPrinter::printTypeHint(size_t id, unsigned char sss)
{
}

void odfutil::DocumentDebugPrinter::printPrimitive(ODF::TypeSpecifier tspec)
{
	using TS = ODF::TypeSpecifier;
	TS::Type t = tspec.smallType();

	if (t == TS::FLAG_BYTE)
	{
		if (tspec.isUnsigned())
			oss << (unsigned int)document.read();
		else
			oss << (int)document.read();
	}
	else if (t == TS::FLAG_SHORT)
	{
		if (tspec.isUnsigned())
			oss << document.read<unsigned short>();
		else
			oss << document.read<short>();
	}
	else if (t == TS::FLAG_INT)
	{
		if (tspec.isUnsigned())
			oss << document.read<unsigned int>();
		else
			oss << document.read<int>();
	}
	else if (t == TS::FLAG_LONG)
	{
		if (tspec.isUnsigned())
			oss << document.read<unsigned long long>();
		else
			oss << document.read<long long>();
	}
	else if (t == TS::FLAG_STRING)
	{
		if (tspec.isWidestring())
			oss << "L'" << to_utf8(document.readWstr()) << "'";
		else
			oss << "'" << document.readStr() << "'";
	}
}

void odfutil::DocumentDebugPrinter::process()
{
	while (document.sizeLeft())
	{
		// print first byte, which is a type specifier
		ODF::TypeSpecifier tspec = document.read();
		printTypeSpec(tspec);

		if (isPrimitive(tspec))
		{
			printPrimitive(tspec);
			continue;
		}
		else if (isVirtual(tspec))
		{
			printVirtual(tspec);
		}
	}
	if (optOss)
		output = optOss->str(); // optOss and oss are the same if optOss is not nullptr
}

odfutil::DocumentDebugPrinter::DocumentDebugPrinter(MemoryDataStream& doc, bool stringout) : optOss(nullptr),
	idx(0),
	document(doc),
	oss(stringout ? (*reinterpret_cast<std::ostream*>(optOss = new std::ostringstream)) : std::cout)
	{}

odfutil::DocumentDebugPrinter::~DocumentDebugPrinter()
{
	if (optOss)
		delete optOss;
}

void odfutil::DocumentDebugPrinter::print(MemoryDataStream& doc)
{
	DocumentDebugPrinter printer(doc);
	printer.process();
	std::cout << printer.output << "\n";
}
