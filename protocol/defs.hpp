#pragma once

#include <iostream>

// TODO better logging system
// maybe write these to a file?
#define LOG_ERR(MSG) std::cerr << MSG <<std::endl
#define LOG(MSG) std::cout << MSG <<std::endl


#ifdef FEDIY_DEBUG
    #if defined(__GNUC__) || defined(__clang__)
        #define DEBUG_LOG(MSG) std::cout << __PRETTY_FUNCTION__ << ": " << MSG <<std::endl
    #else
        #define DEBUG_LOG(MSG) std::cout << __func__ << ": " << MSG <<std::endl
    #endif
#else
    #define DEBUG_LOG(MSG) void()
#endif
