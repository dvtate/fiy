//
// Created by tate on 4/10/26.
//

#include "ModConnectorDll.hpp"

#include <dlfcn.h>

#include <nlohmann/json.hpp>

#include "../../modlib/fiymod.hpp"

#include "../FIY.hpp"

#include "../Server/util.hpp"

#include "../LocalUser.hpp"
#include "../Server/Session.hpp"

#include "../../util/ThreadPool.hpp"

struct ModConnectorDll::ModDLLHostInfo : fiy::fiy_host_info_t {
    // TODO these should probably be unique_ptr's ?
    std::string m_base_uri;

    explicit ModDLLHostInfo(const Mod* mod) {
        this->log = [](const int n, const char* s){
            static const char* types[] = { "FATAL", "ERROR", "WARN", "INFO", "DEBUG" };
            const char* type_str;
            if (n > 4 || n < 0) {
                LOG_WARN("Mod used fiy_host_info.log with invalid log type");
                type_str = "INVALID";
            } else {
                type_str = types[n];
            }

            std::cerr <<"Mod: " <<type_str <<": " <<s <<std::endl;
        };
        this->now = []() { return g_fiy->now(); };

        m_base_uri =  g_fiy->m_config.m_protocol;
        m_base_uri += g_fiy->m_config.m_hostname;
        m_base_uri += '/';
        m_base_uri += mod->m_path;

        this->base_uri = m_base_uri.c_str(); // host info shouldn't be moved either
        this->request = ModDLLHostInfo::request_impl;
        this->domain = g_fiy->m_config.m_hostname;
        this->app_id = mod->m_id.c_str(); // safe assuming the mod isn't moved
        this->local_login = ModDLLHostInfo::local_login_impl;
        this->user_info = ModDLLHostInfo::user_info_impl;
        this->data_dir = mod->m_user_data_dir.c_str();
        this->mod_config = mod->m_config.c_str();
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
        const char* app_id,
        const fiy::fiy_request_t* request,
        void* context,
        void (*callback)(const struct fiy::fiy_response_t*, void*)
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
    static int user_info_impl(const char* local_user_name, fiy::fiy_local_user_info_t* ret) {
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

/// Message passed to shared library
struct ModDllConnectorRequest : public fiy::fiy_request_t {
    std::shared_ptr<Session> m_conn;

    using RequestHandler = void (*)(struct fiy_request_t*, fiy::fiy_callback_t);
    RequestHandler on_request;

    explicit ModDllConnectorRequest(std::shared_ptr<Session> conn, RequestHandler on_request):
        ModDllConnectorRequest(std::move(conn), conn->find_user(), on_request)
    {}

    explicit ModDllConnectorRequest(
        std::shared_ptr<Session> conn,
        const Session::User& user,
        RequestHandler handle_request
    ):
        m_conn(std::move(conn)),
        on_request(handle_request)
    {
        auto& r = m_conn->req();

        // Initialize fiy_request_t
        this->method = static_cast<uint8_t>(r.method());
        // TODO should use string_view equivalent to avoid copies
        this->path = new_cstr_from_string(r.target());
        this->body = r.body().data(); // this should be safe bc we have shared
        this->body_len = r.body().size();
        this->domain = user.domain;
        this->user = (user.user.empty() || user.user == " ")
                     ? nullptr
                     : new_cstr_from_string(user.user);
        set_this_headers(m_conn->req().base());
    }

    virtual ~ModDllConnectorRequest() {
        delete[] this->path;
        delete[] this->user;
        delete[] this->headers;
    }

    void callback(const fiy::fiy_response_t* r) {
        switch (r->body.type) {
            case fiy::BodyType::FIY_BODY_NONE: {
                Session::EmptyResponse res;
                res.result(r->status);
                response_set_headers(res, r->headers);
                m_conn->respond(m_conn->prep(std::move(res)));
                break;
            }

            case fiy::BodyType::FIY_BODY_FILE: {
                // Convert to boost file type
                boost::beast::file_posix fp;
                fp.native_handle(r->body.file.fd);
                boost::beast::error_code ec;
                if (r->body.file.offset != 0) {
                    fp.seek(r->body.file.offset, ec);
                    if (ec) {
                        LOG_ERR("Seek failed: " << ec.message());
                        break;
                    }
                }

                // Construct response
                Session::FileResponse res;
                res.body().reset(std::move(fp), ec); // note: have to use reset or else it will set size = 0
                if (ec) {
                    LOG_ERR("Failed to set file: " << ec.message());
                    Session::StringResponse resp;
                    resp.body() = "Server Error: Failed to set body";
                    resp.result(500);
                    m_conn->respond(m_conn->prep(std::move(resp)));
                    break;
                }
                res.result(r->status);
                response_set_headers(res, r->headers);
                m_conn->respond(m_conn->prep(std::move(res)));
                break;
            }

            case fiy::BodyType::FIY_BODY_BUFFER:
            case fiy::BodyType::FIY_BODY_READER: {
                Session::StringResponse res;
                res.body() = fiy::Body::to_string(r->body);
                res.result(r->status);
                response_set_headers(res, r->headers);
                m_conn->respond(m_conn->prep(std::move(res)));
                break;
            }
        }
    }

    static char* new_cstr_from_string(const std::string_view s) {
        const auto l = s.size();
        char* ret = new char[l + 1];
        if (!s.empty())
            memcpy(ret, s.data(), l);
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
            // TODO maybe some headers we should skip?
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

bool ModConnectorDll::stop() {
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

bool ModConnectorDll::start() {
    // Generate host info
    delete m_host_info;
    m_host_info = new ModDLLHostInfo(m_mod);

    // Open shared object
    if (m_dl_handle != nullptr) {
        DEBUG_LOG("Handle replaced!");
    }
    m_dl_handle = dlopen(m_uri.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (m_dl_handle == nullptr) {
        DEBUG_LOG(this->m_mod->m_name <<": dlopen failed: " <<dlerror());
        return false;
    }

    // Start mod
    const auto start_fn = (fiy::StartFunction) dlsym(m_dl_handle, "start");
    if (start_fn == nullptr) {
        DEBUG_LOG(this->m_mod->m_name <<": could not find start function -- " <<dlerror());
        return false;
    }
    m_mod_info = start_fn(m_host_info);
    if (m_mod_info == nullptr) {
        DEBUG_LOG(this->m_mod->m_name <<": start() returned null");
        return false;
    }

    // Update fields
    if (m_mod_info->version != nullptr /* && !m_mod->m_version.initialized() */)
            m_mod->m_version = Mod::Version(m_mod_info->version);
    if (m_mod_info->id != nullptr && m_mod->m_id.empty())
        m_mod->m_id = m_mod_info->id;
    return true;
}

void ModConnectorDll::delete_user(const char* user) {
    if (m_mod_info == nullptr || m_mod_info->delete_user == nullptr)
        return;
    m_mod_info->delete_user(user);
}

void ModConnectorDll::handle_request(std::shared_ptr<Session> conn) {
    // Check access restrictions
    const auto user = conn->find_user();
    const char* username = user.user.empty() ? nullptr : user.user.c_str();
    if (m_mod->m_can_access == nullptr
        || !m_mod->m_can_access(username, user.domain)
    ) {
        static const auto body_str = "<h1>401 - Unauthorized</h1><hr/><a href='"
            + g_fiy->base_uri() + "/portal'>Go to portal</a>";
        Session::StringResponse res;
        res.result(401);
        res.body() = body_str;
        conn->respond(conn->prep(res));
        return;
    }

    // Safety check
    if (m_mod_info == nullptr || m_mod_info->on_request == nullptr) {
        boost::beast::http::response<boost::beast::http::string_body> res;
        res.result(500);
        res.body() = "Module not initialized";
        conn->respond(conn->prep(std::move(res)));
        DEBUG_LOG("Mod " <<m_mod->m_id <<": called before start()");
        return;
    }

    // Call mods in separate threadpool so that they don't block asio threads
    static ThreadPool<std::unique_ptr<ModDllConnectorRequest>> request_handler{
        [](std::unique_ptr<ModDllConnectorRequest> r) {
            r->on_request(
                &*r,
                [](
                    const fiy::fiy_request_t* req,
                    const fiy::fiy_response_t* resp
                ){
                    const auto r = (ModDllConnectorRequest*) req;
                    r->callback(resp); // this deletion of r
                }
            );
        },
        g_fiy->m_config.m_concurrency
    };

    // Send it to the thread pool
    request_handler.emplace(new ModDllConnectorRequest(std::move(conn), user, m_mod_info->on_request));
}

void ModConnectorDll::handle_request(
    const fiy::fiy_request_t* req,
    void* context,
    void (*callback)(const struct fiy::fiy_response_t*, void*)
) {
    // Augmented request object with callback+context
    // TODO maybe should just add a callback member to fiy_request_t
    //      would simplify APIs a bit and we wouldn't need this:
    struct ModDllConnectorRequestWrapper : public fiy::fiy_request_t {
        ModDllConnectorRequestWrapper(
            const fiy::fiy_request_t& req,
            void (*callback)(const fiy::fiy_response_t*, void* context),
            void* context
        ): fiy::fiy_request_t(req), m_callback(callback), m_context(context)
        {}

        void callback(const fiy::fiy_response_t* res) const {
            m_callback(res, m_context);
            delete this;
        }
    private:
        void (*m_callback)(const fiy::fiy_response_t*, void* context);
        void* m_context;
    };

    // Send request to mod
    fiy_request_t* r = new ModDllConnectorRequestWrapper(*req, callback, context);
    m_mod_info->on_request(
        r,
        [](
            const fiy::fiy_request_t* req,
            const fiy::fiy_response_t* resp
        ){
            ((ModDllConnectorRequestWrapper*) req)->callback(resp);
        }
    );
}
