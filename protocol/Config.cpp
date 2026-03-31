#include "Config.hpp"

#include <thread>
#include <filesystem>

#include "../third_party/inih/ini.h"

#include "../util/Crypto.hpp"
#include "../util/FileCache.hpp"

#include "Pages.hpp"

/**
 * Parse config file
 * @param path path to the ini config file to parse
 * @return false on error
 */
bool INIConfig::parse(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        LOG("Config file " << path << " does not exist!");
        m_error = true;
        return false;
    }

    LOG("Loading Config file: " << path);
    m_error = ini_parse(
        path.c_str(),
        [](void* cfg, auto section, auto key, auto value){
            int ret = ((INIConfig*) cfg)->set_key(section, key, value);
            if (!ret) {
                LOG_ERR("Config file: invalid key: " <<key);
            }
            return ret;
        },
        this
    );
    return !m_error;
}

bool FediyConfig::from_file(const std::string& path) {
    const bool ret = parse(path);
    set_defaults();
    return ret;
}

bool FediyConfig::from_argv(int argc, char** argv) {
    // TODO use a proper command line library instead
    if (argc <= 1)
        return from_file(CONFIG_FILE_PATH);

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            from_file(argv[i]);
            set_defaults();
        }
        const char* p = argv[i];
        while (*p == '-') p++;

        if (strcmp(p, "help")) {
            std::cout <<"Usage: " <<argv[0] <<" [CONFIG_FILE_PATH] [CONFIG_OVERRIDES]...\n\n"
                "\tConfig overrides have the same names as in the config file but must be prepended with '--'";
            continue;
        }

        if (i + 1 >= argc) {
            LOG_ERR("Option -" << p << ": missing value");
            return false;
        }
        if (!set_key("", p, argv[i]))
            return false;
    }
    set_defaults();
    return true;
}


bool FediyConfig::set_key(const char* section, const char* key, const char* value) {
    // No section
    if (section[0] != '\0') {
        LOG_ERR("Config file: invalid section: " <<section);
        return false;
    }

    if (strcmp(key, "data_dir") == 0) {
        // Where app files are stored
        std::error_code ec;
        if (!std::filesystem::is_directory(value, ec)) {
            LOG_ERR("Config file: data_dir invalid path: " << value);
        }
        if (ec.value() != 0) {
            LOG_ERR("filesystem::is_directory failed: " << ec.message());
        }
        m_data_dir = value;
    } else if (strcmp(key, "hostname") == 0) {
        // Validate
        // TODO better regex that includes localhost
        const std::regex domain_name_pattern{ // supports subdomains and port numbers
            R"(^(?!-)[A-Za-z0-9-]+([\-\.]{1}[a-z0-9]+)*\.[A-Za-z]{2,6}(?:\:[0-9]{1,5})?$)"};
        if (!std::regex_match(value, domain_name_pattern)) {
            LOG_ERR("Config file: hostname '" << value << "' may be invalid (valid example: example.com)");
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
        m_public_key = Pages::load_file_as_string(value);
        if (m_public_key.empty()) {
            LOG_ERR("Config file: public_key: could not read file: " << value);
        }
    } else if (strcmp(key, "private_key") == 0) {
        m_private_key = Crypto::SSL::load_private_key_from_pem(
            Pages::load_file_as_string(value)
        );
        if (m_private_key == nullptr) {
            LOG_ERR("Config file: private_key: could not load file: " << value);
        }
    } else if (strcmp(key, "address") == 0) {
        m_listen_addr = value;
    } else {
        LOG_ERR("Invalid key: " <<key);
        return false; // invalid key
    }
    return true;
}

// Call after parse to fill in any missing items
void FediyConfig::set_defaults() {
    // Try to get keys from their default locations
    if (m_public_key.empty()) {
        const std::string path = m_data_dir + "/auth/pubkey.crt";
        m_public_key = Pages::load_file_as_string(path);
        if (m_public_key.empty()) {
            LOG_ERR("Config file: public_key: could not read file: " << path);
        }
    }
    if (m_private_key == nullptr) {
        const std::string path = m_data_dir + "/auth/privkey.pem";
        m_private_key = Crypto::SSL::load_private_key_from_pem(
            Pages::load_file_as_string(path)
        );
        if (m_private_key == nullptr) {
            LOG_ERR("Config file: private_key: could not load file: " << path);
        }
    }
}