//
// Created by tate on 7/30/25.
//

#pragma once

#include <string>
#include <fstream>
#include <string_view>

#include "../../../util/FileCache.hpp"
#include "../../../util/MinSSR.hpp"

#include "../../../modlib/fiymod.hpp"

// TODO replace this with FileCache?

static std::string get_frontend_dir() {
    return std::string(fiy::host().data_dir) + "/assets/";
}

struct Pages : FileCache<get_frontend_dir> {
    /**
     * Pre-render global data
     */
    static std::string render_global_data(const std::string& templ) {
#define FIY_MOD_CONTACTS_PAGES_GLOBAL_RULES(kv) \
            kv("fiy_contacts_base_uri", fiy::host().base_uri) \
            kv("fiy_contacts_domain", fiy::host().domain)

        return MIN_SSR_MUSTACHE_VARIABLE_TEMPLATE(templ, FIY_MOD_CONTACTS_PAGES_GLOBAL_RULES);
#undef FIY_MOD_CONTACTS_PAGES_GLOBAL_RULES
    }

    /**
     * Get cached contents of file (with substitutions) as a string
     * @tparam FileSubPath static const char* data dir subpath
     * @return string contents of the file
     */
    template<const char* FileSubPath>
    static const std::string& file_contents() {
        static const std::string contents = render_global_data(get_file_contents<FileSubPath>());
        return contents;
    }

    /**
     * Body containing file contents w/ global subs
     */
    template<const char* FileSubPath>
    static fiy::Body file_body() {
        return fiy::Body(file_contents<FileSubPath>());
    }

    /**
     * Body containing raw file contents
     */
    template<const char* FileSubPath>
    static fiy::Body mm_file_body() {
        return fiy::Body(std::string_view(mm_file<FileSubPath>()));
    }

    static fiy::Body main_js() {
        static const char path[] = "main.bundle.js";
        return file_body<path>();
    }

    static fiy::Body index_html() {
        static const char path[] = "index.html";
        return file_body<path>();
    }

    static fiy::Body main_css() {
        static const char path[] = "main.css";
        return mm_file_body<path>();
    }
};
