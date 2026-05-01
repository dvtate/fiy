#pragma once

#include <string>
#include <filesystem>

#include <nlohmann/json.hpp>

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
    std::string config;
    std::string data_dir;
    std::string id;
    std::string path;
    std::string name;
    std::string description;
    std::string icon;
    std::string user_data_dir;
    std::filesystem::file_time_type install_ts;
    Version version{0, 0};
    AccessChecker can_access{nullptr};

    // Communicate with mod
    std::unique_ptr<ModConnector> ipc{nullptr};

    Mod() = default;
    explicit Mod(const std::string& dir);
    ~Mod();

    bool start();
    bool stop();

    void set_enabled(bool enabled);
    bool is_enabled() const { return m_enabled; }
    bool is_loaded() const { return m_loaded; }
    void set_path(const std::string& new_path);


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

protected:
    void load_error(const std::string& message);

    static AccessChecker parse_access_checker(const std::string& value);
};
