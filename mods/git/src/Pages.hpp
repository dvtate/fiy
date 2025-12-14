//
// Created by tate on 12/3/25.
//

#ifndef FEDIY_PAGES_HPP
#define FEDIY_PAGES_HPP

#include <utility>

#include "../../../modlib/fediymod.hpp"
#include "../../../util/FileCache.hpp"

extern fiy::HostInfo g_host_info;

inline std::string get_frontend_dir() {
    return g_host_info.data_dir + std::string("/static");
}

struct Pages : FileCache<get_frontend_dir> {

    static ReplacementMap get_host_data() {
        return {
            { "{{fiy_domain}}", g_host_info.domain },
            { "{{fiy_baseuri}}", g_host_info.base_uri },
        };
    }

    /// Automatically replace host_data in the loaded template
    template<const char* FileSubPath>
    static const std::string& file_contents() {
        return FileCache<get_frontend_dir>::file_contents<FileSubPath>(get_host_data());
    }
    template<const char* FileSubPath>
    static fiy::Body file_body() {
        return fiy::Body(file_contents<FileSubPath>());
    }
};

#endif //FEDIY_PAGES_HPP