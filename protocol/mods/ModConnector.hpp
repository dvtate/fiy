//
// Created by tate on 7/5/24.
//

#pragma once

#include <string>
#include <memory>

#include "../defs.hpp"

#include "../../modlib/fiymod.h"

class Session;
class Mod;

/**
 * Abstract class that handles communication between the protocol host and mods
 */
// TODO we should track requests, if 500+ requests awaiting response, we should maybe warn the user
class ModConnector {
public:
    /// Owner
    Mod* m_mod{nullptr};

    /// Where can we connect to it?
    std::string m_uri;

    ModConnector(Mod* mod, std::string path): m_mod(mod), m_uri(std::move(path)) {}

    // owning mod should call stop before destructor
    virtual ~ModConnector() = default;

    /// Initialize the app
    virtual bool start() = 0;

    /// Cleanly shut the app down
    virtual bool stop() = 0;

    /// Handle user request
    virtual void handle_request(std::shared_ptr<Session> conn) = 0;
    virtual void handle_request(
        const fiy::fiy_request_t* req,
        void* context,
        void (*callback)(const fiy::fiy_response_t*, void*)
    ) = 0;

    /// Handle user data deletion request
    virtual void delete_user(const char* user) = 0;

    /// IPC interface
    enum class Type {
        SHARED_LIBRARY,     // .so file
        NETWORK,            // HTTP/HTTPS
        // CGI,                // TODO common gateway interface
    };

    [[nodiscard]] virtual Type type() = 0;
};

struct ModDLLHostInfo;

/// Communicates with the module by dynamically linking
class ModDLLConnector : public ModConnector {
    void* m_dl_handle{nullptr};
    fiy::fiy_mod_info_t* m_mod_info{nullptr};
    ModDLLHostInfo* m_host_info{nullptr};

public:
    ModDLLConnector(Mod* mod, std::string path): ModConnector(mod, std::move(path)) {}

    Type type() override { return Type::SHARED_LIBRARY; }
    bool stop() override;
    bool start() override;
    void delete_user(const char* user) override;
    void handle_request(std::shared_ptr<Session> conn) override;
    void handle_request(
        const fiy::fiy_request_t* req,
        void* context,
        void (*callback)(const fiy::fiy_response_t*, void*)
    ) override;
};

// IPC over the network
class ModNetConnector : public ModConnector {
    std::string m_bearer_send;
    std::string m_bearer_accept;
    bool m_https;

public:
    // TODO uri could have a subpath component that needs to be separated
    ModNetConnector(Mod* mod, std::string path, const bool is_https = false):
        ModConnector(mod, std::move(path)),
        m_https(is_https)
    {}

    Type type() final { return Type::NETWORK; }
    bool start() override;
    bool stop() override;
    void delete_user(const char* user) override;
    void handle_request(std::shared_ptr<Session>) override;
    void handle_request(
        const fiy::fiy_request_t* req,
        void* context,
        void (*callback)(const fiy::fiy_response_t*, void*)
    ) override;
};
