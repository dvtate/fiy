//
// Created by tate on 7/5/24.
//

#include <ranges>
#include <functional>

#include <dlfcn.h>

#include <nlohmann/json.hpp>

#include "ModConnector.hpp"

#include "Server/Session.hpp"
#include "LocalUser.hpp"
#include "FIY.hpp"
#include "../modlib/fediymod.hpp"
#include "Server/util.hpp"

/// Message passed to shared library
struct ModDllConnectorRequest : public fiy_request_t {
    std::shared_ptr<Session> m_conn;

    explicit ModDllConnectorRequest(std::shared_ptr<Session> conn):
        m_conn(std::move(conn))
    {
        auto user = m_conn->find_user();
        auto& r = m_conn->req();

        // Initialize fiy_request_t
        this->method = (uint8_t) r.method();
        this->path = new_cstr_from_string(r.target());
        this->body = new_cstr_from_string(r.body());
        this->body_len = r.body().size();
        this->domain = user.domain;
        this->user = (user.user.empty() || user.user == " ")
                     ? nullptr
                     : new_cstr_from_string(user.user);
        set_this_headers(m_conn->req().base());
    }

    virtual ~ModDllConnectorRequest() {
        delete[] this->body;
        delete[] this->path;
        delete[] this->user;
        delete[] this->headers;
    }

    void remove_from_task_queue() {
        // TODO might be nice to have a list of active tasks and remove
        delete this;
    }

    void callback(const fiy_response_t* r){
        Session::StringResponse res;
        res.body() = std::string(r->body, r->body_len);
        res.result(r->status);
        response_set_headers(res, r->headers);

        // TODO add headers
        m_conn->respond(m_conn->prep(std::move(res)));
        this->remove_from_task_queue();
    }
    static char* new_cstr_from_string(const std::string_view s) {
        const auto l = s.size();
        char* ret = new char[l + 1];
        memcpy(ret, s.data(), l + 1);
        ret[l] = '\0';
        return ret;
    }
    void set_this_headers(const boost::beast::http::fields& f) {
        if (f.cbegin() == f.cend()) {
            this->headers = nullptr;
            return;
        }
        std::string ret;
        for (const auto& h : f) {
            ret += h.name_string();
            ret += ": ";
            ret += h.value();
            ret += "\r\n";
        }

        // Copy but skip trailing \r\n
        const size_t len = ret.size() - 1;
        char* p = new char[len];
        memset(p, 0, len);
        strncpy(p, ret.data(), len - 1);
        this->headers = p;
    }
};

struct ModDLLHostInfo : fiy_host_info_t {
    // TODO these should probably be unique_ptr's ?
    std::string m_base_uri;
    std::string m_data_dir;

    explicit ModDLLHostInfo(Mod* mod) {
        this->log = [](const int n, const char* s){
            static const char* types[] = { "FATAL", "ERROR", "WARN", "INFO", "DEBUG" };
            const char* type_str;
            if (n > 5 || n < 0) {
                std::cerr << "Mod used fiy_host_info.log with invalid log type\n";
                type_str = "INVALID";
            } else {
                type_str = types[n];
            }

            std::cerr <<"Mod: " <<type_str <<": " <<s <<std::endl;
        };
        this->now = []() { return g_fiy->now(); };

        m_base_uri = std::string(
            strchr(g_fiy->m_config.m_hostname, ':') == nullptr ? "https://" : "http://"
            ) + g_fiy->m_config.m_hostname + '/' + mod->m_path;

        this->base_uri = m_base_uri.c_str();
        this->request = ModDLLHostInfo::request_impl;
        this->domain = g_fiy->m_config.m_hostname;
        this->app_id = mod->m_id.c_str(); // safe assuming the mod isn't moved
        this->local_login = ModDLLHostInfo::local_login_impl;
        this->user_info = ModDLLHostInfo::user_info_impl;
        m_data_dir = g_fiy->m_config.m_data_dir + "/mods/" + mod->m_path;
        this->data_dir = m_data_dir.c_str();
    }

    /**
     * Send a request to another app
     * @param app_id app to send the request to
     * @param request request to send to the other app
     *      method   - http method
     *      path     - uri path
     *      domain   - remote server to send request to or nullptr if local inter-app request
     *      user     - local user or nullptr if unauthenticated
     * @param context this pointer is passed back to the callback at the end
     * @param callback called with response
     * @notes
     * - local apps can send requests to each other without restrictions
     * - an app on server a can only send requests to apps on server b on behalf of users residing on server a
     *    - this prevents false impersonation
     */
    static void request_impl(
//    const struct fiy_host_info_t* host,
        const char* app_id,
        const fiy_request_t* request,
        void* context,
        void (*callback)(const struct fiy_response_t*, void*)
    ) {
        // Send request to peer
        g_fiy->m_peers.request_peer(request->domain, app_id, request, context, callback);
    }

    /**
     * Authenticate an instance-local user
     * @param username user's username
     * @param password user's password
     * @return
     *  0 on success
     *  1 if the user does not exist
     *  -1 error
     */
    static int local_login_impl(const char* username, const char* password) {
        try {
            auto u = DB::get_user(username, password);
            return u != nullptr ? 0 : 1;
        } catch (const DB::Exception& e) {
            LOG_ERR("DB Error: " <<e.what());
            return -1;
        }
    }

    /**
     * Get information for a given local user
     * @param local_user_name username for relevant user
     * @param ret struct to put the results into, or null to check existence
     * @return
     *  0 on success
     *  1 if the user does not exist
     *  -1 error
     */
    static int user_info_impl(const char* local_user_name, fiy_local_user_info_t* ret) {
        const auto u = DB::get_user(local_user_name);
        if (u == nullptr)
            return 1;
        if (ret == nullptr)
            return 0;

        // Check sizes
        if (u->get_name().size() >= sizeof(ret->name) / sizeof(char)
            || u->m_locale.size() >= sizeof(ret->locale) / sizeof(char)
        ) {
            LOG_ERR("ModDLLHostInfo: content would overflow field");
            return -1;
        }

        strcpy(ret->name, u->get_name().c_str());
        strcpy(ret->locale, u->m_locale.c_str());
        ret->admin = u->m_is_admin;
        ret->join_ts = u->m_joined_ts;
        return 0;
    }
};

bool ModDLLConnector::stop() {
    // Nothing to stop
    if (m_dl_handle == nullptr)
        return true;

    // Call stop function
    const auto stop_fn = (void(*)()) dlsym(m_dl_handle, "start");
    if (stop_fn != nullptr)
        stop_fn();

    // Close handle
    auto ret = dlclose(m_dl_handle);
    if (ret == 0) {
        m_dl_handle = nullptr;
    } else {
        LOG_ERR("dlclose() gave" << ret <<": " <<dlerror());
    }

    // Cleanup
    delete m_host_info;
    return ret == 0;
}

bool ModDLLConnector::start() {
    // Generate host info
    delete m_host_info;
    m_host_info = new ModDLLHostInfo(m_mod);

    // Open shared object
    if (m_dl_handle != nullptr) {
        DEBUG_LOG("Handle replaced!");
    }
    m_dl_handle = dlopen(m_uri.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (m_dl_handle == nullptr)
        return false;

    // Start mod
    const auto start_fn = (fiy_mod_start_function_t) dlsym(m_dl_handle, "start");
    m_mod_info = start_fn(m_host_info);
    if (m_mod_info == nullptr)
        return false;

    // Update fields
    if (m_mod_info->version != nullptr /* && !m_mod->m_version.initialized() */)
            m_mod->m_version = Mod::Version(m_mod_info->version);
    if (m_mod_info->id != nullptr && m_mod->m_id.empty())
        m_mod->m_id = m_mod_info->id;
    return true;
}

void ModDLLConnector::delete_user(const char* user) {
    if (m_mod_info == nullptr || m_mod_info->delete_user == nullptr)
        return;
    m_mod_info->delete_user(user);
}

void ModDLLConnector::handle_request(std::shared_ptr<Session> conn) {
    // Safety check
    if (m_mod_info == nullptr || m_mod_info->on_request == nullptr) {
        boost::beast::http::response<boost::beast::http::string_body> res;
        res.result( 500 );
        res.body() = "Module not initialized";
        conn->respond(conn->prep(std::move(res)));
        DEBUG_LOG("Mod " <<m_mod->m_id <<": called before start()");
        return;
    }

    fiy_request_t* r = new ModDllConnectorRequest(conn);
    m_mod_info->on_request(
        r,
        [](
            const fiy_request_t* req,
            const fiy_response_t* resp
        ){
            ((ModDllConnectorRequest*) req)->callback(resp);
        }
    );
}

void ModDLLConnector::handle_request(
    const fiy_request_t* req,
    void* context,
    void (*callback)(const struct fiy_response_t*, void*)
) {
    struct ModDllConnectorRequestWrapper : public fiy_request_t {
        ModDllConnectorRequestWrapper(
            const fiy_request_t& req,
            void (*callback)(const fiy_response_t*, void* context),
            void* context
        ): fiy_request_t(req), m_callback(callback), m_context(context)
        {}

        void callback(const fiy_response_t* res) const {
            m_callback(res, m_context);
            delete this;
        }
    private:
        void (*m_callback)(const fiy_response_t*, void* context);
        void* m_context;
    };

    fiy_request_t* r = new ModDllConnectorRequestWrapper(*req, callback, context);
    m_mod_info->on_request(
        r,
        [](
            const fiy_request_t* req,
            const fiy_response_t* resp
        ){
            ((ModDllConnectorRequestWrapper*) req)->callback(resp);
        }
    );
}


bool ModNetConnector::start() {
    // JSON payload containing fields: bearer token, mod id, base_uri, now
    do {
        m_bearer_accept = PeerAuth::get_token_string();
    } while (!g_fiy->m_mods.add_net_connector(m_bearer_accept, this));
    const std::string payload = "{\"bearer\":\"" + m_bearer_accept
        + "\",\"id\":\""+ m_mod->m_id
        + "\",\"base_uri\":\"" + (
            strchr(g_fiy->m_config.m_hostname, ':') == nullptr ? "https://" : "http://"
        ) + g_fiy->m_config.m_hostname + '/' + m_mod->m_path
        + "\",\"now\":" + std::to_string(g_fiy->now()) + "}";

    // Generate request
    using namespace boost::beast;
    http::request<http::string_body> req;
    req.method(http::verb::post);
    req.target("/start");
    req.body() = payload;
    req.set("Fiy-Signature", Crypto::SSL::sign(g_fiy->m_config.m_private_key, payload));
    req.keep_alive(false);
    req.prepare_payload();

    // Callback handler
    auto cb = [this](http::response<http::string_body> res) {
        std::string s = res.body();
        auto j = nlohmann::json::parse(s);
        if (!j.is_object()) {
            m_mod->error("bad response, should be an object");
            return;
        }
        if (!j.contains("accept") && j["accept"].is_string()) {
            m_mod->error("bad response, missing 'accept' field ");
            return;
        } else {
            m_bearer_send = j["accept"].get<std::string>();
        }
        if (j.contains("id")
            && m_mod->m_id.empty()
            && j["id"].is_string()
        ) {
            m_mod->m_id = j["id"].get<std::string>();
        }
        if (j.contains("version")
            // && !m_mod->m_version.initialized()
            && j["version"].is_string()
        ) {
            m_mod->m_version = Mod::Version(j["version"].get<std::string>());
        }
    };
    auto err_cb = [this](const std::string& err) {
        std::cerr << "Mod " << m_mod->m_id <<": Failed to connect to " <<m_uri <<std::endl;
        m_mod->error(err);
    };

    // Make request
    if (m_https)
        g_fiy->m_https.request(m_uri, req, cb, err_cb);
    else
        g_fiy->m_http.request(m_uri, req, cb, err_cb);

    // hostinfo should also include authentication system
    // ie - credentials we can validate for every request
    return true;
}

bool ModNetConnector::stop() {
    // Already stopped
    if (m_bearer_accept.empty()) {
        DEBUG_LOG("Already stopped");
        return true;
    }

    g_fiy->m_mods.remove_net_connector(m_bearer_accept);
    m_bearer_accept.clear();

    using namespace boost::beast;
    http::request<http::empty_body> req;
    req.method(http::verb::post);
    req.target("/stop");
    req.set("Fiy-Auth", m_bearer_send);
    req.keep_alive(false);

    auto cb = [this](auto resp) {
        if (resp.result() != http::status::ok)
            DEBUG_LOG("Mod " <<m_mod->m_id <<": failed to stop: HTTP " <<resp.result());
        else
            DEBUG_LOG("Mod " <<m_mod->m_id <<": stopped");
    };
    auto err_cb = [this](auto err) {
        DEBUG_LOG("Mod " <<m_mod->m_id <<": failed to stop: " <<err);
        (void)err;
    };

    if (m_https)
        g_fiy->m_https.request(m_uri, req, cb, err_cb);
    else
        g_fiy->m_http.request(m_uri, req, cb, err_cb);
    return true;
}

void ModNetConnector::handle_request(std::shared_ptr<Session> conn) {
    // TODO probably shouldn't send the fiy_auth cookie
    //  but mods should be trusted so not concerning

    conn->req().set("Fiy-User", conn->find_user().str());
    conn->req().set("Fiy-Auth", m_bearer_send);
    std::string target = "/request";
    target += conn->req().target();
    conn->req().target() = target;

    auto cb = [session = std::move(conn)] (auto resp) {
        session->respond(std::move(resp));
    };
    auto err_cb = [this, session = std::move(conn)] (std::string err) {
        Session::StringResponse res;
        res.result(500);
        res.body() = "<h1>Server Error</h1><p>Request failed: " + err + "</p>";
        res.set(boost::beast::http::field::content_type, "text/html");
        session->respond(session->prep(std::move(res)));
        LOG_ERR("ModNetConnector[" <<m_mod->m_id <<"]: ");
    };

    if (m_https)
        g_fiy->m_https.request(m_uri, conn->req(), cb, err_cb);
    else
        g_fiy->m_http.request(m_uri, conn->req(), cb, err_cb);
}

void ModNetConnector::delete_user(const char* user) {
    boost::beast::http::request<boost::beast::http::empty_body> req;
    req.target("/user");
    req.method(boost::beast::http::verb::delete_);
    req.set("Fiy-Auth", m_bearer_send);
    req.set("Fiy-User", user);
    req.keep_alive(false);

    auto cb = [this] (auto resp) {
        if (resp.result_int() != 200)
            LOG("Mod " <<m_mod->m_id <<": failed to delete user: HTTP code " <<resp.result());
    };
    auto err_cb = [this, user] (auto err) {
        LOG("Mod " <<m_mod->m_id <<": failed to delete user: " <<user <<" -- " <<err);
    };

    if (m_https)
        g_fiy->m_https.request(m_uri, req, cb, err_cb);
    else
        g_fiy->m_http.request(m_uri, req, cb, err_cb);
}

void ModNetConnector::handle_request(
    const fiy_request_t* req,
    void* context,
    void (*callback)(const fiy_response_t*, void*)
) {
    const fiy::Request& r = *req;
    namespace http = boost::beast::http;
    http::request<http::string_body> net_req;
    net_req.method(http::verb::post);
    net_req.target() = std::string("/request") + r.path;
    net_req.set("Fiy-Auth", m_bearer_send);
    net_req.set("Fiy-User", r.user);
    net_req.set("Fiy-Domain", r.domain);
    net_req.set("Fiy-Method",
        http::to_string(static_cast<http::verb>(r.method)));
    for ( const auto& [k, v] : r.headers_map())
        net_req.set(k, v);
    net_req.body() = r.body;
    net_req.keep_alive(false);
    net_req.prepare_payload();

    auto cb = [context, callback] (auto resp) {
        if (callback == nullptr)
            return;

        // Convert headers into a string
        std::string headers_str;
        for (const auto& f : resp.base()) {
            headers_str += f.name_string();
            headers_str += ": ";
            headers_str += f.value();
            headers_str += '\n';
        }

        const fiy_response_t response{
            .status = static_cast<int>(resp.result_int()),
            .headers = headers_str.c_str(),
            .body_len = resp.body().size(),
            .body = resp.body().data(),
        };
        callback(&response, context);
    };
    auto err_cb = [this, context, callback] (auto err) {
        DEBUG_LOG("Mod " <<m_mod->m_id <<": handle request failed " <<err);
        if (callback == nullptr)
            return;

        const fiy_response_t response{
            .status = -1,
            .headers = "",
            .body_len = err.size(),
            .body = err.c_str(),
        };
        callback(&response, context);
    };

    if (m_https)
        g_fiy->m_https.request(m_uri, net_req, cb, err_cb);
    else
        g_fiy->m_http.request(m_uri, net_req, cb, err_cb);
}
