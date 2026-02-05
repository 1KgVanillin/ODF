#pragma once

#ifdef __BASE_FILE__
#define __FILENAME__ __BASE_FILE__
#else
#define __FILENAME__ __FILE__
#endif

#ifdef _DEBUG
#define rethrow std::cout << "\033[31m" << "[Sus] Exception triggered in " << __FILENAME__ << " line " << __LINE__ << " (rethrow)\033[0m\n"; throw
#define THROW std::cout << "\033[31m" << "[Sus] Exception triggered in " << __FILENAME__ << " line " << __LINE__ << "\033[0m\n"; throw
#else
#define rethrow
#define THROW throw
#endif

struct InvalidDefault {};