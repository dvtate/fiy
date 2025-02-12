//
// Created by tate on 7/5/24.
//

#pragma once

#include <string>
#include <functional>
#include <dlfcn.h>

#include "drogon/HttpController.h"
#include "drogon/HttpClient.h"

#include "globals.hpp"

#include "../modlib/fediymod.h"

#include "HTTPRoutes/ModuleRoutes.hpp"

class Mod;

// Abstract class
class ModIPC {
public:
    Mod* m_mod{nullptr};
    std::string m_ipc_uri;
    ModIPC(Mod* mod, std::string path): m_mod(mod), m_ipc_uri(std::move(path)) {}
    virtual ~ModIPC() = default;

    virtual bool start() = 0;
    virtual bool stop() = 0;
    virtual void handle_request(
            const drogon::HttpRequestPtr& req,
            ModuleRoutes::User&& user,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback
    ) = 0;


    // IPC interface
    enum class IPCType {
        SHARED_LIBRARY,     // .so file
        SOCKET,             // unix socket connection
        NETWORK             // tcp connection
    };

    [[nodiscard]] virtual IPCType ipc_type() = 0;
};

// Message passed to shared library
class ModDllIpcRequest : public fediy::fiy_request_t {
public:
    ModuleRoutes::User m_user;
    std::function<void(const drogon::HttpResponsePtr&)> m_callback;

    ModDllIpcRequest(
        const drogon::HttpRequestPtr& req,
        ModuleRoutes::User&& user,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
    );

    virtual ~ModDllIpcRequest() {
        delete[] this->body;
        delete[] this->path;
    }

    void remove_from_task_queue() {
        // TODO might be nice to have a list of active tasks and remove
        delete this;
    }

    void callback(const fediy::fiy_response_t* r) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setBody(r->body);
        resp->setStatusCode((drogon::HttpStatusCode)r->status);
        // TODO add headers
//        resp->addHeader()
        m_callback(resp);

        this->remove_from_task_queue();
    }
};

// Communicates with the module by dynamically linking
class ModDLLIPC : public ModIPC {
    void* m_dl_handle{nullptr};
    fediy::fiy_mod_info_t* m_mod_info{nullptr};
    fediy::fiy_host_info_t* m_host_info{nullptr};

    void gen_host_info();
    void free_host_info() {
        if (m_host_info != nullptr) {
            delete m_host_info->base_uri;
            delete m_host_info;
            m_host_info = nullptr;
        }
    }

//    std::list<ModDllIpcRequest*> m_msg_queue;

public:
    ModDLLIPC(Mod* mod, std::string path): ModIPC(mod, std::move(path)) {}

    ~ModDLLIPC() {
        ModDLLIPC::stop();
    }

    bool stop() override {
        if (m_dl_handle == nullptr)
            return true;
        auto ret = dlclose(m_dl_handle);
        if (ret == 0) {
            m_dl_handle = nullptr;
        } else {
            LOG_ERR("dlclose() gave" << ret <<": " <<dlerror());
        }
        free_host_info();
        return ret == 0;
    }

    bool start() override {
        gen_host_info();
        if (m_dl_handle != nullptr) {
            DEBUG_LOG("Handle replaced!");
        }
        m_dl_handle = dlopen(m_ipc_uri.c_str(), RTLD_LAZY | RTLD_LOCAL);
        if (m_dl_handle == nullptr)
            return false;

        auto start_fn = (fediy::fiy_mod_start_function_t) dlsym(m_dl_handle, "start");
        m_mod_info = start_fn(m_host_info);
        return m_mod_info != nullptr;
    }

    IPCType ipc_type() {
        return IPCType::SHARED_LIBRARY;
    }

    void handle_request(
        const drogon::HttpRequestPtr& req,
        ModuleRoutes::User&& user,
        std::function<void(const drogon::HttpResponsePtr&)>&& callback
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

    virtual void handle_request(
            const drogon::HttpRequestPtr& req,
            ModuleRoutes::User&& user,
            std::function<void(const drogon::HttpResponsePtr&)>&& callback
    ) override {
        auto client = drogon::HttpClient::newHttpClient(m_ipc_uri);
        req->addHeader("fediy-user", user.user + '@' + user.domain);
        client->sendRequest(
            req,
            [
                this,
                cb = std::move(callback)
            ](
                drogon::ReqResult status,
                const drogon::HttpResponsePtr& resp
            ) {
//                if (status == drogon::ReqResult::Ok) {
                if (resp != nullptr) {
                    resp->setPassThrough(true);
                    cb(resp);
                } else {
                    auto r = drogon::HttpResponse::newHttpResponse();
                    r->setStatusCode(drogon::k500InternalServerError);
                    cb(r);
                }
            }
        );
    };
};

// IPC over unix socket
class ModSockIPC : public ModIPC {
public:
    ModSockIPC(Mod* mod, std::string path): ModIPC(mod, std::move(path)) {}

    IPCType ipc_type() final {
        return IPCType::SOCKET;
    }

    // TODO
};