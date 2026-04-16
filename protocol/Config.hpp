#pragma once

#include <string>
#include <fstream>
#include <cctype>
#include <cinttypes>
#include <filesystem>
#include <cstring>
#include <regex>
#include <openssl/types.h>

#include "defs.hpp"


/**
 * INI Config file abstract base class
 */
class INIConfig {
public:
    virtual ~INIConfig() = default;

    bool error{false};

    bool parse(const std::string& path);

protected:
    virtual bool set_key(const char* section, const char* key, const char* value) = 0;

    static int parse_bool(const char* str) {
        const auto len = strlen(str);
        if (len == 4)
            return strcasecmp(str, "true") == 0;
        if (len == 5)
            return strcasecmp(str, "false") == 0;
        if (len >= 1) {
            if (str[0] == 'y' || str[0] == 't' || str[0] == 'Y' || str[0] == 'T' || str[0] == '1')
                return true;
            if (str[0] == 'n' || str[0] == 'f' || str[0] == 'N' || str[0] == 'F' || str[0] == '0')
                return false;
        }
        return -1;
    }
};

/**
 * Parse protocol server config file
 */
class FiyConfig : public INIConfig {
public:
    // TODO move to /etc
    static constexpr const char* CONFIG_FILE_PATH = "/opt/fiy/config.ini";

    FiyConfig() = default;
    explicit FiyConfig(const std::string& path) {
        parse(path);
        set_defaults(); // call after parse
    }

    /// Where files are stored
    std::string data_dir{"/opt/fiy"};

    /// Domain where we're hosting the service
    char* hostname{nullptr};

    /// Password salt that should not be changed
    // if we have a store with purchases we can give the user a key that only works with their salt
    // if they try to change their salt to that of another user they'll get locked out
    // install package generates admin account
    std::string salt;

    /// TCP port number to bind to
    int port{8848};

    /// Hint for the number of threads to use
    int concurrency{4};

    /// Server public key
    std::string public_key;

    /// Server private key
    EVP_PKEY* private_key{nullptr};

    /// Listen address
    std::string listen_addr;

    const char* protocol{nullptr};

    [[nodiscard]] bool https() const {
        return protocol == PROTOCOL_HTTPS;
    }

    // TODO request timeouts?

    bool from_argv(int argc, char** argv);
    bool from_file(const std::string& path = CONFIG_FILE_PATH);

protected:
    bool set_key(const char* section, const char* key, const char* value) override;
    void set_defaults();

    static constexpr const char* PROTOCOL_HTTP = "http://";
    static constexpr const char* PROTOCOL_HTTPS = "https://";
};

