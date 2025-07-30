//
// Created by tate on 7/5/24.
//

#include <ranges>

#include "ModIPC.hpp"

#include "LocalUser.hpp"
#include "App.hpp"

#include "Server/util.hpp"


static char* new_cstr_from_string(const std::string_view& s) {
    auto l = s.size();
//    std::cout <<"new cstr: '" << s <<"' -- Size: " <<l <<std::endl;
    char* ret = new char[l + 1];
    strncpy(ret, s.data(), l);
    ret[l] = '\0';
    return ret;
}

ModDllIpcRequest::ModDllIpcRequest(std::shared_ptr<Session> conn):
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
    this->headers = nullptr;
}


void ModDllIpcRequest::callback(const fiy_response_t* r) {
    Session::StringResponse res;
    res.body() = std::string(r->body, r->body_len);
    res.result(r->status);
    response_set_headers(res, r->headers);

    // TODO add headers
    m_conn->respond(m_conn->prep(std::move(res)));
    this->remove_from_task_queue();
}

void send_request_to_app(
//    const struct fiy_host_info_t* host,
    const char* app_id,
    const fiy_request_t* request,
    void* context,
    void (*callback)(const struct fiy_response_t*, void*)
) {
    // Send request to peer
    g_app->m_peers.request_peer(request->domain, app_id, request->user, request, context, callback);
}

void ModDLLIPC::gen_host_info() {
    if (m_host_info != nullptr)
        free_host_info();

    m_host_info = new fiy_host_info_t;
    m_host_info->log = [](int n, const char* s){
        std::cout <<"Mod: " <<s <<std::endl;
    };
    std::string base_uri = std::string(g_app->m_config.m_ssl ? "https://" : "http://")
        + g_app->m_config.m_hostname + '/' + m_mod->m_path;

    char* cstr = new char[base_uri.size() + 1];
    strcpy(cstr, base_uri.c_str());
    m_host_info->base_uri = cstr;
    m_host_info->request = send_request_to_app;
    m_host_info->domain = g_app->m_config.m_hostname;
    m_host_info->app_id = m_mod->m_id.c_str();
}

void ModDLLIPC::handle_request(std::shared_ptr<Session> conn) {
    fiy_request_t* r = new ModDllIpcRequest(conn);
    m_mod_info->on_request(
        r,
        [](
            const fiy_request_t* req,
            const fiy_response_t* resp
        ){
            ((ModDllIpcRequest*) req)->callback(resp);
        }
    );
}

void ModDLLIPC::handle_request(
    const fiy_request_t* req,
    void* context,
    void (*callback)(const struct fiy_response_t*, void*)
) {
    struct ModDllIpcRequestWrapper : public fiy_request_t {
        ModDllIpcRequestWrapper(
            const fiy_request_t& req,
            void (*callback)(const fiy_response_t*, void* context),
            void* context
        ): fiy_request_t(req), m_callback(callback), m_context(context)
        {}

        void callback(const fiy_response_t* res) {
            m_callback(res, m_context);
            delete this;
        }
    private:
        void (*m_callback)(const fiy_response_t*, void* context);
        void* m_context;
    };

    fiy_request_t* r = new ModDllIpcRequestWrapper(*req, callback, context);
    m_mod_info->on_request(
        r,
        [](
            const fiy_request_t* req,
            const fiy_response_t* resp
        ){
            ((ModDllIpcRequestWrapper*) req)->callback(resp);
        }
    );
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
    g_app->m_http.request(
        m_ipc_uri,
        conn->req(),
        [session = std::move(conn)] (auto resp) {
            session->respond(std::move(resp));
        }
    );
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