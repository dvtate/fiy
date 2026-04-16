#pragma once

#include <string>
#include <filesystem>

#include "nlohmann/json.hpp"

#include "../../util/Version.hpp"

#include "ModConnector.hpp"

class Mods;

/**
 * Installed mod interface
 *
 * @note Apps and Mods refer to the same thing but referred to as Mods in the code to reduce ambiguity
 */
class Mod {
protected:
    std::mutex m_mtx; // for operations on the module config

    // Runtime data
    bool m_loaded{true};
    bool m_enabled{false};
    bool m_running{false};
    std::string m_error;
public:

    enum class Status {
        INVALID,    // failed to read module
        DISABLED,   // module installed but not enabled
        ENABLED,    // module enabled but not started
        RUNNING,    // module running
        FAILED,     // module failed while running
    };

    using AccessChecker = std::function<bool(const char* user, const char* domain)>;

    // Metadata
    std::string m_config;
    std::string m_data_dir;
    std::string m_id;
    std::string m_path;
    std::string m_name;
    std::string m_description;
    std::string m_icon;
    std::string m_user_data_dir;
    std::filesystem::file_time_type m_install_ts;
    Version m_version{0, 0};
    AccessChecker m_can_access{[](const char*, const char*){ return true; }};

    // Communicate with mod
    std::unique_ptr<ModConnector> m_ipc{nullptr};

    Mod() = default;
    explicit Mod(std::string data_dir);
    ~Mod();

    bool start();
    bool stop();

    void set_enabled(bool enabled);
    void set_path(const std::string& path);


    [[nodiscard]] Status status() const {
        if (!m_loaded)
            return Status::INVALID;
        if (!m_enabled)
            return Status::DISABLED;
        if (m_running)
            return Status::RUNNING;
        if (m_error.empty())
            return Status::ENABLED;
        return Status::FAILED;
    }

    // Probably shouldn't let the user edit module.json like this
    std::string json();
    std::string user_json();
    nlohmann::json parse_file();
    void save();
    void load();

    void error(const std::string& err) {
        m_error = err;
        m_running = false;
    }

    inline std::filesystem::path dir() const;

    friend class Mods;

protected:
    void load_error(const std::string& message);

    static AccessChecker parse_access_checker(const std::string& value);
};
