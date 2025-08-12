#pragma once

#include <string>
#include <filesystem>

#include "defs.hpp"

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

    // overengineering
    struct Version {
        mutable long major{-1}, minor{-1};
        mutable std::string major_string;

        Version() = default;

        explicit Version(const std::string& version_str) {
            std::size_t pos;
            major = std::stoi(version_str, &pos);
            if (pos >= version_str.size() || version_str.at(pos) != '.') {
                minor = 0;
                return;
            }
            char* end = nullptr;
            minor = std::strtol(version_str.c_str() + pos, &end, 10);
            major_string = std::to_string(major);
        }

        Version(long major, long minor):
                major(major), minor(minor)
        {
            major_string = std::to_string(major);
        }

        [[nodiscard]] bool compatible(const Version& other) const {
            return major == other.major;
        }
        [[nodiscard]] bool compatible(const std::string& other_major_str) const {
            return major_string == other_major_str;
        }

        [[nodiscard]] auto operator<=>(const Version& other) const {
            if (auto c = major <=> other.major; c != 0)
                return c;
            return minor <=> other.minor;
        }
        [[nodiscard]] std::string str() const {
            return major_string + '.' + std::to_string(minor);
        }
    };

    // Metadata
    std::string m_id;
    std::string m_name;
    std::string m_path;
    std::string m_description;
    std::string m_icon;
    std::filesystem::file_time_type m_install_ts;
    Version m_version;
    std::string m_daemon; // make it a BackgroundProcess ?

    // Communicate with mod
    std::unique_ptr<ModConnector> m_ipc{nullptr};

    Mod() = default;
    explicit Mod(std::string id);
    ~Mod();

    bool start();
    bool stop();

    void set_enabled(bool enabled);
    void set_path(const std::string& id);

    enum class Status {
        INVALID,    // failed to read module
        DISABLED,   // module installed but not enabled
        ENABLED,    // module enabled but not started
        RUNNING,    // module running
        FAILED,     // module failed while running
    };

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

    std::string json();
    std::string user_json();
    void save();

    inline std::filesystem::path dir() const;

    friend class Mods;
};
