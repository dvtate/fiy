//
// Created by tate on 7/5/24.
//

#include <ranges>

#include "ModIPC.hpp"

#include "App.hpp"
#include "LocalUser.hpp"

#include "Server/util.hpp"

// Message passed to shared library
class ModDllIpcRequest : public fiy_request_t {
public:
    std::shared_ptr<Session> m_conn;

    explicit ModDllIpcRequest(std::shared_ptr<Session> conn) : m_conn(std::move(conn)) {
        auto user = m_conn->find_user();
        auto& r = m_conn->req();

        // Initialize fiy_request_t
        this->method = (uint8_t)r.method();
        this->path = new_cstr_from_string(r.target());
        this->body = new_cstr_from_string(r.body());
        this->body_len = r.body().size();
        this->domain = user.domain;
        this->user =
            (user.user.empty() || user.user == " ") ? nullptr : new_cstr_from_string(user.user);
        this->headers = nullptr;
    }

    virtual ~ModDllIpcRequest() {
        delete[] this->body;
        delete[] this->path;
        delete[] this->user;
    }

    void remove_from_task_queue() {
        // TODO might be nice to have a list of active tasks and remove
        delete this;
    }

    void callback(const fiy_response_t* r) {
        Session::StringResponse res;
        res.body() = std::string(r->body, r->body_len);
        res.result(r->status);
        response_set_headers(res, r->headers);

        // TODO add headers
        m_conn->respond(m_conn->prep(std::move(res)));
        this->remove_from_task_queue();
    }
    static char* new_cstr_from_string(const std::string_view s) {
        auto l = s.size();
        //    std::cout <<"new cstr: '" << s <<"' -- Size: " <<l <<std::endl;
        char* ret = new char[l + 1];
        strncpy(ret, s.data(), l);
        ret[l] = '\0';
        return ret;
    }
};

struct ModDLLHostInfo : fiy_host_info_t {
    std::string m_base_uri;

    explicit ModDLLHostInfo(Mod* mod) {
        this->log = [](int n, const char* s) { std::cout << "Mod: " << s << std::endl; };

        m_base_uri = std::string(g_app->m_config.m_ssl ? "https://" : "http://") +
                     g_app->m_config.m_hostname + '/' + mod->m_path;

        this->base_uri = m_base_uri.c_str();
        this->request = ModDLLHostInfo::request_impl;
        this->domain = g_app->m_config.m_hostname;
        this->app_id = mod->m_id.c_str();  // safe assuming the mod isn't moved
        this->local_login = ModDLLHostInfo::local_login_impl;
        this->user_info = ModDLLHostInfo::user_info_impl;
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
     * - an app on server a can only send requests to apps on server b on behalf of users residing
     * on server a
     *    - this prevents false impersonation
     */
    static void request_impl(
        //    const struct fiy_host_info_t* host,
        const char* app_id, const fiy_request_t* request, void* context,
        void (*callback)(const struct fiy_response_t*, void*)) {
        // Send request to peer
        g_app->m_peers.request_peer(request->domain, app_id, request, context, callback);
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
            LOG_ERR("DB Error: " << e.what());
            return -1;
        }
    }

    /**
     * Get information for a given local user
     * @param local_user_name username for relevant user
     * @param ret struct to put the results into
     * @return
     *  0 on success
     *  1 if the user does not exist
     *  -1 error
     */
    static int user_info_impl(const char* local_user_name, fiy_local_user_info_t* ret) {
        if (ret == nullptr)
            return -1;

        auto u = DB::get_user(local_user_name);
        if (u == nullptr)
            return 1;

        // Check sizes
        if (u->get_name().size() >= sizeof(ret->name) / sizeof(char) ||
            u->m_locale.size() >= sizeof(ret->locale) / sizeof(char)) {
            LOG_ERR("ModDLLHostInfo: content would overflow field");
            return -1;
        }

        strcat(ret->name, u->get_name().c_str());
        strcat(ret->locale, u->m_locale.c_str());
        ret->admin = u->m_is_admin;
        ret->join_ts = u->m_joined_ts;
        return 0;
    }
};

bool ModDLLIPC::stop() {
    if (m_dl_handle == nullptr)
        return true;
    auto ret = dlclose(m_dl_handle);
    if (ret == 0) {
        m_dl_handle = nullptr;
    } else {
        LOG_ERR("dlclose() gave" << ret << ": " << dlerror());
    }
    delete m_host_info;
    return ret == 0;
}

bool ModDLLIPC::start() {
    if (m_host_info != nullptr)
        delete m_host_info;
    m_host_info = new ModDLLHostInfo(m_mod);
    if (m_dl_handle != nullptr) {
        DEBUG_LOG("Handle replaced!");
    }
    m_dl_handle = dlopen(m_ipc_uri.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (m_dl_handle == nullptr)
        return false;

    auto start_fn = (fiy_mod_start_function_t)dlsym(m_dl_handle, "start");
    m_mod_info = start_fn(m_host_info);
    return m_mod_info != nullptr;
}

void ModDLLIPC::handle_request(std::shared_ptr<Session> conn) {
    fiy_request_t* r = new ModDllIpcRequest(conn);
    m_mod_info->on_request(r, [](const fiy_request_t* req, const fiy_response_t* resp) {
        ((ModDllIpcRequest*)req)->callback(resp);
    });
}

void ModDLLIPC::handle_request(const fiy_request_t* req, void* context,
                               void (*callback)(const struct fiy_response_t*, void*)) {
    struct ModDllIpcRequestWrapper : public fiy_request_t {
        ModDllIpcRequestWrapper(const fiy_request_t& req,
                                void (*callback)(const fiy_response_t*, void* context),
                                void* context)
            : fiy_request_t(req), m_callback(callback), m_context(context) {}

        void callback(const fiy_response_t* res) {
            m_callback(res, m_context);
            delete this;
        }

    private:
        void (*m_callback)(const fiy_response_t*, void* context);
        void* m_context;
    };

    fiy_request_t* r = new ModDllIpcRequestWrapper(*req, callback, context);
    m_mod_info->on_request(r, [](const fiy_request_t* req, const fiy_response_t* resp) {
        ((ModDllIpcRequestWrapper*)req)->callback(resp);
    });
}

bool ModNetIPC::start() {
    // if server uri is localhost/127.0.0.1 then start the server

    // Assume the other server is already running
    // send hostinfo as json
    // expect modinfo as json

    // hostinfo should also include authentication system
    // ie - credentials we can validate for every request
    return true;
}

bool ModNetIPC::stop() {
    return true;
}

void ModNetIPC::handle_request(std::shared_ptr<Session> conn) {
    conn->req().set("Fediy-User", conn->find_user().str());
    g_app->m_http.request(m_ipc_uri, conn->req(), [session = std::move(conn)](auto resp) {
        session->respond(std::move(resp));
    });
}

//    virtual void handle_request(
//            const drogon::HttpRequestPtr& req,
//            User&& user,
//            std::function<void(const drogon::HttpResponsePtr&)>&& callback
//    ) override {
//        auto client = drogon::HttpsClient::newHttpClient(m_ipc_uri);
//        req->addHeader("fediy-user", user.user + '@' + user.domain);
//        client->sendRequest(
//            req,
//            [
//                this,
//                cb = std::move(callback)
//            ](
//                drogon::ReqResult status,
//                const drogon::HttpResponsePtr& resp
//            ) {
////                if (status == drogon::ReqResult::Ok) {
//                if (resp != nullptr) {
//                    resp->setPassThrough(true);
//                    cb(resp);
//                } else {
//                    auto r = drogon::HttpResponse::newHttpResponse();
//                    r->setStatusCode(drogon::k500InternalServerError);
//                    cb(r);
//                }
//            }
//        );
//    }