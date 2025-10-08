//
// Created by tate on 7/31/25.
//

#ifndef FEDIY_FILECACHE_HPP
#define FEDIY_FILECACHE_HPP

#include <string>
#include <fstream>
#include <iostream>

struct FileCache {
protected:
    std::string m_basedir;

public:
    explicit FileCache(std::string path_prefix): m_basedir(std::move(path_prefix)) {}

    static std::string load_file_as_string(const std::string& file_path) {
        // Open the file
        std::ifstream f{file_path};
        if (!f.is_open()) {
            std::string log_msg = "Error: Could not open file ";
            log_msg += file_path;
            std::cerr <<log_msg <<std::endl;
            return ""; // Return an empty string on failure
        }

        // Create a string big enough for the file
        std::string ret;
        f.seekg(0, std::ios::end);
        ret.reserve(1 + (ssize_t) f.tellg());
        f.seekg(0, std::ios::beg);

        // Read entire file content into return string
        ret.assign(std::istreambuf_iterator<char>(f),
                   std::istreambuf_iterator<char>());
        return ret;
    }

    /// Get cached contents of file as a string
    template<const char* FileSubPath>
    [[nodiscard]] const std::string& file_contents() const {
        static const std::string contents = load_file_as_string(m_basedir + FileSubPath);
        return contents;
    }

    /// Getter
    [[nodiscard]] const std::string& prefix() const {
        return m_basedir;
    }
};

#endif //FEDIY_FILECACHE_HPP
