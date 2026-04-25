#pragma once
#include "ODF.h"

namespace odfutil
{
	// finish later TODO
	class DocumentDebugPrinter
	{
		struct Color
		{
			static constexpr const char* Default = "\x1b[0m";
			static constexpr const char* Red = "\x1b[0;31m";
			static constexpr const char* Green = "\x1b[0;32m";
			static constexpr const char* Yellow = "\x1b[0;33m";
			static constexpr const char* Blue = "\x1b[0;34m";
			static constexpr const char* Magenta = "\x1b[0;35m";
			static constexpr const char* Cyan = "\x1b[0;36m";
			static constexpr const char* White = "\x1b[0;37m";

			static constexpr const char* Underline_Red = "\x1b[1;31m";
			static constexpr const char* Underline_Green = "\x1b[1;32m";
			static constexpr const char* Underline_Yellow = "\x1b[1;33m";
			static constexpr const char* Underline_Blue = "\x1b[1;34m";
			static constexpr const char* Underline_Magenta = "\x1b[1;35m";
			static constexpr const char* Underline_Cyan = "\x1b[1;36m";
			static constexpr const char* Underline_White = "\x1b[1;37m";

			static constexpr const char* Italic_Red = "\x1b[3;31m";
			static constexpr const char* Italic_Green = "\x1b[3;32m";
			static constexpr const char* Italic_Yellow = "\x1b[3;33m";
			static constexpr const char* Italic_Blue = "\x1b[3;34m";
			static constexpr const char* Italic_Magenta = "\x1b[3;35m";
			static constexpr const char* Italic_Cyan = "\x1b[3;36m";
			static constexpr const char* Italic_White = "\x1b[3;37m";

			static constexpr const char* Decent_Red = "\x1b[2;31m";
			static constexpr const char* Decent_Green = "\x1b[2;32m";
			static constexpr const char* Decent_Yellow = "\x1b[2;33m";
			static constexpr const char* Decent_Blue = "\x1b[2;34m";
			static constexpr const char* Decent_Magenta = "\x1b[2;35m";
			static constexpr const char* Decent_Cyan = "\x1b[2;36m";
			static constexpr const char* Decent_White = "\x1b[2;37m";

			static constexpr const char* Keyword = Italic_Green;
			static constexpr const char* HardValue = Yellow;
			static constexpr const char* Suffix = Decent_Cyan;
			static constexpr const char* Special = Italic_Magenta;
			static constexpr const char* TypeSpec = Blue;
			static constexpr const char* TypeInfo = Decent_Blue;
			static constexpr const char* TypeInfoKey = Decent_Magenta;
			static constexpr const char* Warning = Underline_Red;
			static constexpr const char* Hint = Decent_Yellow;
		};

		std::unordered_map<ODF::VTypeInfo::TypeID, std::string, ODF::VTypeInfo::TypeIDHasher> typeHints;

		size_t idx; // curret index
		MemoryDataStream& document;
		std::string output;
		std::ostream& oss;
		std::ostringstream* optOss;
		std::string newline;
		unsigned short indent;
		void makeNewline(); // creates newline from indent
		void addIndent();
		void removeIndent();

		static bool isPrimitive(ODF::TypeSpecifier tspec);
		void printPrimitive(ODF::TypeSpecifier tspec);

		static bool isVirtual(ODF::TypeSpecifier tspec);
		void printVirtual(ODF::TypeSpecifier tspec);

		void printTypeSpec(ODF::TypeSpecifier tspec, bool asKeyword = true);
		void printComplexType();
		void printTypeHint(size_t id, unsigned char sss);


		void process();

		DocumentDebugPrinter(MemoryDataStream& doc, bool stringout = false); // stringout= true-> print to string (output) else, print to stdout
		~DocumentDebugPrinter();
	public:

		static void print(MemoryDataStream& doc);
	};
}