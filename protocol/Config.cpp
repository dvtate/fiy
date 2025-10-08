#include <thread>
#include <filesystem>
#include "../third_party/inih/ini.h"

#include "Config.hpp"

#include "../util/Crypto.hpp"
#include "../util/FileCache.hpp"

bool Config::parse(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        LOG("Config file " << path << " does not exist!");
        return m_error = true;
    }

    LOG("Loading Config file: " << path);
    return m_error = ini_parse(
        path.c_str(),
        [](void* cfg, auto section, auto key, auto value){
            return (int)((Config*) cfg)->set_key(section, key, value);
        },
        this
    );
}

bool FediyConfig::set_key(const char* section, const char* key, const char* value) {
    // No section
    if (section[0] != '\0')
        return false;

    if (strcmp(key, "data_dir") == 0) {
        // Where app files are stored
        std::error_code ec;
        if (!std::filesystem::is_directory(value, ec)) {
            LOG_ERR("Config file: data_dir invalid path: " << value << std::endl);
        }
        if (ec.value() != 0) {
            LOG_ERR("filesystem::is_directory failed: " << ec.message() << std::endl);
        }
        m_data_dir = value;
    } else if (strcmp(key, "hostname") == 0) {
        // Validate
        const std::regex domain_name_pattern{ // supports subdomains and port numbers
            R"(^(?!-)[A-Za-z0-9-]+([\-\.]{1}[a-z0-9]+)*\.[A-Za-z]{2,6}(?:\:[0-9]{1,5})?$)"};
        if (!std::regex_match(value, domain_name_pattern)) {
            LOG_ERR("Config file: hostname '" << value << "' is invalid (valid example: example.com)");
        }

        // Apply
        m_hostname = (char*) malloc(strlen(value) + 1);
        strcpy(m_hostname, value);
    } else if (strcmp(key, "salt") == 0) {
        m_salt = value;
    } else if (strcmp(key, "port") == 0) {
        int port = strtol(value, nullptr, 10);
        if (port == 0 || port < 0 || port >= 65536) {
            LOG_ERR("Config file: port should be a valid port number, not " << value);
        } else {
            m_port = port;
        }
    } else if (strcmp(key, "concurrency") == 0) {
        char* pend = nullptr;
        int threads = strtol(value, &pend, 10);
        if (pend == value) {
            LOG_ERR("Config file: concurrency should be an integer, failed to parse " << value);
            threads = 0;
        }
        if (threads <= 0) {
            m_concurrency = std::thread::hardware_concurrency();
            if (-threads >= m_concurrency)
                m_concurrency = 1;
            else
                m_concurrency += threads;
        } else {
            m_concurrency = threads;
        }
    } else if (strcmp(key, "public_key") == 0) {
        m_public_key = FileCache::load_file_as_string(value);
        if (m_public_key.empty()) {
            LOG_ERR("Config file: public_key: could not read file: " << value);
        }
    } else if (strcmp(key, "private_key") == 0) {
        m_private_key = Crypto::SSL::load_private_key_from_pem(
            FileCache::load_file_as_string(value)
        );
        if (m_private_key == nullptr) {
            LOG_ERR("Config file: private_key: could not load file: " << value);
        }
    } else {
        LOG_ERR("Config file: invalid key: " <<key);
        return false; // invalid key
    }
    return true;
}

// Call after parse to fill in any missing items
void FediyConfig::set_defaults() {
    // Try to get keys from their default locations
    if (m_public_key.empty()) {
        const std::string path = m_data_dir + "/auth/pubkey.crt";
        m_public_key = FileCache::load_file_as_string(path);
        if (m_public_key.empty()) {
            LOG_ERR("Config file: public_key: could not read file: " << path);
        }
    }
    if (m_private_key == nullptr) {
        const std::string path = m_data_dir + "/auth/privkey.pem";
        m_private_key = Crypto::SSL::load_private_key_from_pem(
            FileCache::load_file_as_string(path)
        );
        if (m_private_key == nullptr) {
            LOG_ERR("Config file: private_key: could not load file: " << path);
        }
    }
}
