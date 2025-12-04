//
// Created by tate on 12/3/25.
//

#ifndef FEDIY_PAGES_HPP
#define FEDIY_PAGES_HPP

#include "../../../modlib/fediymod.hpp"
#include "../../../util/FileCache.hpp"

extern fiy::HostInfo g_host_info;

inline std::string get_frontend_dir() {
    return g_host_info.data_dir + std::string("/static");
}

struct Pages : FileCache<get_frontend_dir> {};


#endif //FEDIY_PAGES_HPP