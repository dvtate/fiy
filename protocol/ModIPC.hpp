//
// Created by tate on 7/5/24.
//

#pragma once

#include <string>
#include <functional>
#include <dlfcn.h>

#include <boost/beast.hpp>

#include "globals.hpp"

#include "../modlib/fediymod.h"

#include "Server/Session.hpp"

class Mod;

/**
 * Abstract class that handles communication between the protocol host and the apps apps
 */
class ModIPC {
public:
    /// Relevant apps
    Mod* m_mod{nullptr};

    std::string m_ipc_uri;

    ModIPC(Mod* mod, std::string path): m_mod(mod), m_ipc_uri(std::move(path)) {}
    virtual ~ModIPC() = default;

    /// Initialize the app
    virtual bool start() = 0;

    /// Cleanly shut the app down
    virtual bool stop() = 0;

    /// Handle user request
    virtual void handle_request(std::shared_ptr<Session> conn) = 0;
    virtual void handle_request(
        const fiy_request_t* req,
        void* context,
        void (*callback)(const fiy_response_t*, void*)
    ) = 0;

    // IPC interface
    enum class IPCType {
        SHARED_LIBRARY,     // .so file
        SOCKET,             // unix socket connection
        NETWORK             // tcp connection
    };

    [[nodiscard]] virtual IPCType ipc_type() = 0;
};


// Communicates with the module by dynamically linking
struct ModDLLHostInfo;

class ModDLLIPC : public ModIPC {
    void* m_dl_handle{nullptr};
    fiy_mod_info_t* m_mod_info{nullptr};
    ModDLLHostInfo* m_host_info{nullptr};

public:
    ModDLLIPC(Mod* mod, std::string path): ModIPC(mod, std::move(path)) {}

    ~ModDLLIPC() {
        ModDLLIPC::stop();
    }

    IPCType ipc_type() override {
        return IPCType::SHARED_LIBRARY;
    }

    bool stop() override;
    bool start() override;
    void handle_request(std::shared_ptr<Session> conn) override;
    void handle_request(
        const fiy_request_t* req,
        void* context,
        void (*callback)(const fiy_response_t*, void*)
    ) override;
};

// IPC over the network
class ModNetIPC : public ModIPC {
    std::string m_auth_secret;
public:
    ModNetIPC(Mod* mod, std::string path): ModIPC(mod, std::move(path)) {}

    IPCType ipc_type() final {
        return IPCType::NETWORK;
    }

    virtual bool start() override;
    virtual bool stop() override;
        // Invalidate credentials both ways


    // TODO
    void handle_request(std::shared_ptr<Session>) override;
    void handle_request(
        const fiy_request_t* req,
        void* context,
        void (*callback)(const fiy_response_t*, void*)
    ) override {}
};

// IPC over unix socket
// there's no reason to use this as it's worse performance than dll
// and can't be done over network (right?)
//class ModSockIPC : public ModIPC {
//public:
//    ModSockIPC(Mod* mod, std::string path): ModIPC(mod, std::move(path)) {}
//
//    IPCType ipc_type() final {
//        return IPCType::SOCKET;
//    }
//
//    // TODO
//};