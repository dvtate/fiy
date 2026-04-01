#pragma once

#include <iostream>

// Version
#define FIY_VERSION_MAJOR 0
#define FIY_VERSION_MINOR 1
#define FIY_VERSION_STRING #FIY_VERSION_MAJOR "." #FIY_VERSION_MINOR

// TODO better logging system
#define LOG_ERR(MSG) std::cerr << MSG <<std::endl
#define LOG_WARN(MSG) std::cerr << MSG <<std::endl
#define LOG(MSG) std::cout << MSG <<std::endl

#ifdef FIY_DEBUG
#   if defined(__GNUC__) || defined(__clang__)
#       define DEBUG_LOG(MSG) std::cout << __PRETTY_FUNCTION__ << ": " << MSG <<std::endl
#   else
#       define DEBUG_LOG(MSG) std::cout << __func__ << ": " << MSG <<std::endl
#   endif
#else
#   define DEBUG_LOG(MSG) void()
#endif
