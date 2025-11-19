#pragma once

#ifdef _DEBUG
#define rethrow std::cout << "\033[31m" << "[Sus] Exception triggered in " << __FILE__ << " line " << __LINE__ << " (rethrow)\033[0m\n"; throw
#define THROW std::cout << "\033[31m" << "[Sus] Exception triggered in " << __FILE__ << " line " << __LINE__ << "\033[0m\n"; throw
#else
#define rethrow
#endif

struct InvalidDefault {};