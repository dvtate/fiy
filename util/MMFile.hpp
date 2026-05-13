//
// Created by tate on 5/8/26.
//

#pragma once

#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <sys/mman.h>

/**
 * Memory-mapped file
 */
class MMFile {
public:
    class Error : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    int m_fd;
    size_t m_size;
    char* m_data;

    MMFile() = delete;
    MMFile(const MMFile&) = delete;
    MMFile& operator=(const MMFile&) = delete;
    MMFile(MMFile&&) = default;

    explicit MMFile(const char* path, const long offset = 0) {
        m_fd = open(path, O_RDONLY);
        if (m_fd < 0) {
#ifdef FIY_DEBUG
            perror("open()");
#endif
            throw Error("Unable to open file");
        }

        // lseek returns the offset from the beginning of the file
        m_size = lseek(m_fd, offset, SEEK_END);
        if (m_size <= 0) {
#ifdef FIY_DEBUG
            perror("lseek()");
#endif
            close(m_fd);
            throw Error("Unable to seek file");
        }

        // we don't need to lseek again as mmap ignores the offset
        m_data = (char *)mmap(NULL, m_size, PROT_READ, MAP_SHARED, m_fd, offset);
        if (m_data == MAP_FAILED) {
#ifdef FIY_DEBUG
            perror("mmap()");
#endif
            close(m_fd);
            throw Error("mmap failed");
        }
    }

    explicit MMFile(const std::string& path, const long offset = 0):
        MMFile(path.c_str(), offset) {}

    ~MMFile() {
        munmap(m_data, m_size);
        close(m_fd);
    }

    const char* data() const {
        return m_data;
    }
    size_t size() const {
        return m_size;
    }

    operator std::string_view() const {
        return std::string_view(m_data, m_size);
    }

    operator const char*() const {
        return m_data;
    }
};
