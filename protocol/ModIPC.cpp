//
// Created by tate on 7/5/24.
//

#include "ModIPC.hpp"

#include "LocalUser.hpp"
#include "App.hpp"

static char* new_cstr_from_string(const std::string_view& s) {
    auto l = s.size();
    char* ret = new char[l + 1];
    strncpy(ret, s.data(), l + 1);
    return ret;
}

ModDllIpcRequest::ModDllIpcRequest(std::shared_ptr<Connection> conn):
    m_conn(std::move(conn))
{
    auto user = conn->find_user();
    // Initialize fiy_request_t
    this->method = (uint8_t) m_conn->req().method();
    this->path = new_cstr_from_string(m_conn->req().target());
    this->body = new_cstr_from_string(m_conn->req().body());
    this->domain = user.domain;
    this->user = (user.user.empty() || user.user == " ")
        ? nullptr
        : new_cstr_from_string(user.user);
    this->headers = nullptr;
}

void ModDllIpcRequest::callback(const fiy_response_t* r) {
    Connection::StringResponse res;
    res.body() = r->body;
    res.result(r->status);
    m_conn->respond(res);
    this->remove_from_task_queue();
}

drogon::HttpMethod drogon_http_method(const std::string& method) {
    if (method == "GET")
        return drogon::HttpMethod::Get;
    if (method == "POST")
        return drogon::HttpMethod::Post;
    if (method == "PATCH")
        return drogon::HttpMethod::Patch;
    if (method == "PUT")
        return drogon::HttpMethod::Put;
    if (method == "DELETE")
        return drogon::HttpMethod::Delete;
    if (method == "HEAD")
        return drogon::HttpMethod::Head;
    if (method == "OPTIONS")
        return drogon::HttpMethod::Options;
    return drogon::HttpMethod::Invalid;
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
}

void ModDLLIPC::handle_request(std::shared_ptr<Connection> conn) {
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