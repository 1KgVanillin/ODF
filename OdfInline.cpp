#include "OdfInline.hpp"
#include <iostream>

std::tuple<std::shared_ptr<char[]>, size_t> oi::CreateRawData(const RawODF& rodf)
{
	// calculate size
	size_t size = 0;
	for (const OIAny& oia : rodf)
	{
		if (const auto ptr = std::get_if<std::vector<Byte>>(&oia))
			size += ptr->size();
		else
			size++;
	}

	std::shared_ptr<char[]> data = std::make_shared<char[]>(size);
	size_t idx = 0;
	for (const OIAny& oia : rodf)
	{
		if (const auto ptr = std::get_if<std::vector<Byte>>(&oia))
			for (const auto it : *ptr)
				data[idx++] = it;
		else
			data[idx++] = std::get<Byte>(oia);
	}

	return std::make_tuple(data, size);
}

std::vector<oi::Byte> oi::operator""strODF(const char* str, size_t len)
{
	std::vector<Byte> data(len + 1);
	memcpy(data.data(), str, len + 1);
	return data;
}
