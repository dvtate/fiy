//
// Created by tate on 6/25/25.
//

#include "../util/Cookie.hpp"

#include "../App.hpp"

#include "Connection.hpp"
#include "Server.hpp"
#include "Router.hpp"


namespace beast = boost::beast;
namespace http = boost::beast::http;

// parse application/x-www-form-urlencoded
bool parse_form_url_encoded(const std::string_view& s, std::deque<std::pair<std::string, std::string>>& ret) {
    size_t i = 0;
    do {
        size_t start = i;
        std::pair<std::string, std::string> kv;
        while (i < s.size() && s[i] != '=')
            i++;
        if (i == s.size() || s[i] != '=') {
            DEBUG_LOG("bad 1");
            return false;
        }
        auto csubstr = s.data();
        kv.first = drogon::utils::urlDecode(csubstr + start, csubstr + i);
        if (kv.first.empty()) {
            DEBUG_LOG("bad 2");
            return false;
        }

        start = ++i;
        while (i < s.size() && s[i] != '&')
            i++;
//        if (i == s.size() || s[i] != '&') {
//            DEBUG_LOG("bad 3");
//            return true;
//        }
        csubstr = s.data();
        kv.second = drogon::utils::urlDecode(csubstr + start, csubstr + i);
        ret.emplace_back(std::move(kv));
        i++;

    } while (i < s.size());
    return true;
}

std::pair<std::string, std::string> parse_app_request_get(std::shared_ptr<Connection> conn) {
    auto path = conn->req().target();
    if (path.starts_with("/mods")) {
        path.remove_prefix(5);
        return { path, conn->req().at("Fiy-Path") };
    }


    auto hostname = conn->req()["Host"];
    if (!hostname.empty()) {
        std::string_view hhn = g_app->m_config.m_hostname;
        if (hostname.ends_with(hhn) && hhn.size() != hostname.size()) {
            // Subdomain app  app.example.com/uri/path
            return {
                hostname.substr(0, hostname.size() - hhn.size() - 1),
                path
            };
        } else if (hostname != hhn) {
            // Invalid request to different host?
//            Connection::DynamicResponse res;
//            res.result(400);
//            boost::beast::ostream(res.body())
//                    << "Wrong host? Expected host to be "
//                    << hhn.data()
//                    << " but host was "
//                    << hostname
//                    << '\n';
//            conn->respond(res);
            std::cerr << "Received request for invalid hostname: expected "
                      << hhn.data()
                      << " but host was "
                      << hostname
                      << '\n';
        }
    }

    // Not a subdomain app  example.com/app/uri/path
    const auto slash_idx = path.find('/', 1);
    std::cout <<"slash_idx = " <<slash_idx <<std::endl;
    if (slash_idx == -1)
        return { path.data() + 1, "/" };
    else
        return { path.substr(1, slash_idx - 1), path.data() + slash_idx };
}

inline static void app_send_msg(std::shared_ptr<Connection> conn) {
    std::cout <<"calling app: " <<conn->req().target() <<std::endl;
    // Get app
    auto [ app, uri ] = parse_app_request_get(conn);

    // Forward to app
    DEBUG_LOG("Calling " <<app <<" : " << uri);
    Mod* m = g_app->m_mods.get_mod(app);
    if (m == nullptr) {
        Connection::DynamicResponse res;
        res.result(404);
        boost::beast::ostream(res.body())
                <<"App '" <<app <<"' not found\n";
        conn->respond(res);
        return;
    }

    conn->req().target(uri);
    m->m_ipc->handle_request(std::move(conn));
    return;
}

Connection::StringResponse signup_page(unsigned status = 200, const std::string& err = "") {
    Connection::StringResponse res;
    res.result(status);
    res.body() = g_app->m_pages->signup_page(err);
    res.set(boost::beast::http::field::content_type, "text/html");
    return res;
}
Connection::StringResponse login_page(unsigned status = 200, const std::string& err = "") {
    Connection::StringResponse res;
    res.result(status);
    res.body() = g_app->m_pages->login_page(err);
    res.set(boost::beast::http::field::content_type, "text/html");
    return res;
}

inline bool str_is_alphanum(const std::string& str) {
    for (const auto& c : str)
        if (!isalnum(c))
            return false;
    return true;
}

void signup_post(std::shared_ptr<Connection> conn) {
    static const auto resp_bad_username = signup_page(400, "Username should contain only alphanumeric characters");
    static const auto resp_username_taken = signup_page(400, "Username is taken, try a different one");
    static const auto resp_bad_form = signup_page(400, "Form Error");
    static const auto resp_long_username = signup_page(400, "Username must be less than 32 characters");
    static const auto resp_long_contact = signup_page(400, "Contact info must be less than 255 characters");

    // Parse form body
    std::deque<std::pair<std::string, std::string>> form;
    if (!parse_form_url_encoded(conn->req().body(), form)) {
        auto res = resp_bad_form;
        conn->respond(res);
        DEBUG_LOG("invalid body: '" <<conn->req().body() <<"'");
        return;
    }
    std::string username, password, contact;
    for (auto&& [k, v] : std::move(form)) {
        if (k == "username")
            username = std::move(v);
        else if (k == "password")
            password = std::move(v);
        else if (k == "contact")
            contact = std::move(v);
        else
            DEBUG_LOG("invalid form field: " <<k);
    }
    DEBUG_LOG("Creating user: " << username <<"  | " << password << " | " << contact );

    // Validate username
    if (!str_is_alphanum(username)) {
        auto res = resp_bad_username;
        conn->respond(res);
        return;
    }
    if (username.size() > 32) {
        auto res = resp_long_username;
        conn->respond(res);
        return;
    }
    if (g_app->m_users.get_username(username) != nullptr) {
        auto res = resp_username_taken;
        conn->respond(res);
        return;
    }

    // Validate contact
    if (contact.size() > 255) {
        auto res = resp_long_contact;
        conn->respond(res);
        return;
    }

    // Create user
    LocalUser user{username, username, false, contact, "en", std::time(0)};
    if (!g_app->m_db->add_user(user, password)) {
        DEBUG_LOG("Failed to create user??");
        auto res = signup_page(500, "Failed to create account, try again later");
        conn->respond(res);
        return;
    }

    auto auth_token = g_app->m_users.login_user(username, password);

    Connection::EmptyResponse res;
    res.result(boost::beast::http::status::temporary_redirect);
    res.set(boost::beast::http::field::set_cookie,
        Cookie::serialize(
            "fiy_auth",
            auth_token.m_token,
            {   .max_age=LocalUser::AuthToken::SESSION_LIFETIME,
                .http_only=true,
                .same_site="true"
            }
        )
    );
    res.set(boost::beast::http::field::location, "/portal");
    conn->respond(res);
}

void login_post(std::shared_ptr<Connection> conn) {
    // Pre-allocated responses
    static const auto resp_get = login_page();
    static const auto resp_bad_form = login_page(drogon::HttpStatusCode::k400BadRequest, "Form Error");
    static const auto resp_bad_creds = login_page(drogon::HttpStatusCode::k401Unauthorized, "Incorrect username/password");

    // Get username and password from body
    std::deque<std::pair<std::string, std::string>> form;
    if (!parse_form_url_encoded(conn->req().body(), form)) {
        auto res = resp_bad_form;
        conn->respond(res);
        DEBUG_LOG("invalid body: '" <<conn->req().body() <<"'");
        return;
    }
    std::string username, password;
    for (auto&& [k, v]: std::move(form)) {
        if (k == "username")
            username = std::move(v);
        else if (k == "password")
            password = std::move(v);
        else {
            DEBUG_LOG("invalid form field: " << k );
        }
    }

    // root user?
//    if (username.empty()) {
//        callback(m_response_invalid_body);
//        DEBUG_LOG("empty username");
//        return;
//    }
//    if (password.empty()) {
//        callback(resp_bad_form);
//        DEBUG_LOG("empty password");
//        return;
//    }


    // Get auth token for user
    auto auth_token = g_app->m_users.login_user(username, password);
    if (auth_token.m_user == nullptr) {
        auto res = resp_bad_creds;
        conn->respond(res);
        DEBUG_LOG("wrong username/password for user " <<username);
        return;
    }

    // Log user in and send them to portal home
    Connection::EmptyResponse res;
    res.result(boost::beast::http::status::temporary_redirect);
    res.set(boost::beast::http::field::location, "/portal");
    res.set(boost::beast::http::field::set_cookie,
        Cookie::serialize(
            "fiy_auth",
            auth_token.m_token,
            {   .max_age=LocalUser::AuthToken::SESSION_LIFETIME,
                .http_only=true,
                .same_site="true"
            }
        )
    );
    conn->respond(res);
}

template <class Str>
auto server_error(const Str& what) {
    Connection::StringResponse res;
    res.set(http::field::content_type, "text/html");
    res.result(http::status::internal_server_error);
    res.body() = "An error occured: '" + std::string(what) + "'";
    return res;
}

std::vector<std::string> split_string(
    const std::string_view& str,
    const std::string& delimiter
) {
    std::vector<std::string> ret;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != std::string::npos) {
        ret.emplace_back(str.substr(prev, pos - prev));
        prev = pos + delimiter.size();
    }

    // To get the last substring (or only, if delimiter is not found)
    ret.emplace_back(str.substr(prev));

    return ret;
}

void peer_handshake(std::shared_ptr<Connection> conn) {
    // TODO this algorithm is insecure and just used for testing
    auto parts = split_string(conn->req().body(), "\n");
    auto domain = parts[0];
    auto token = parts[1];

    std::cout <<"New peer: " <<domain <<" : " <<token <<std::endl;

    // Add peer with unique auth token
    std::shared_ptr<Peer> p;
    do {
        p = std::make_shared<Peer>(domain, PeerAuth{"symkey", token});
    } while (!g_app->m_peers.add_peer(domain, p));

    // Respond with token
    Connection::StringResponse res;
    res.result(200);
    res.body() = p->m_auth.m_bearer_token_we_accept;
    conn->respond(res);

    // Actual algorithm to use:
    // 1. Use private key associated with our pubkey endpoint to decrypt request body
    // 2. Store provided peer data: host, symkey, tokens, etc. into Peers cache
    // 3. Send token + symkey, encrypted with the other peer's pubkey

    // Decrypt body
//    auto encryptedBody = drogon::utils::base64Decode(req->body()); // may contain \0

    // Get relevant public key
//    auto client = drogon::HttpClient::newHttpClient(remote_peer);
//    client.request();

}

void route_request(std::shared_ptr<Connection> conn) {
    auto h = conn->req().at("Host");
    auto path = conn->req().target();
    switch (conn->req().method()) {
        case boost::beast::http::verb::get:
            if (path.starts_with("/portal")) {
                path.remove_prefix(7);
                if (path.starts_with("/login")) {
                    conn->respond(login_page());
                    return;
                } else if (path.starts_with("/signup")) {
                    conn->respond(signup_page());
                    return;
                } else if (path.starts_with("/main.js")) {
                    // Open the file
                    // TODO would probably be better to use string body instead
                    // TODO enable cache
                    beast::error_code ec;
                    boost::beast::http::file_body::value_type body;
                    auto file_path = g_app->m_config.m_data_dir + "/page_templates/main.js";
                    body.open(file_path.c_str(), beast::file_mode::scan, ec);
                    if (ec) {
                        conn->respond(server_error(ec.what()));
                        return;
                    }

                    // Send it
                    Connection::FileResponse res{
                        boost::beast::http::status::ok,
                        conn->req().version(),
                        std::move(body)
                    };
                    res.set(boost::beast::http::field::content_type, "text/javascript");
                    conn->respond(res);
                } else if (path.empty() || path == "/") {
                    auto user = conn->find_user_local();
                    if (user == nullptr) {
                        Connection::EmptyResponse res;
                        res.result(boost::beast::http::status::temporary_redirect);
                        res.set(boost::beast::http::field::location, "/portal/login");
                        conn->respond(res);
                        return;
                    }
                    Connection::StringResponse res;
                    res.result(200);
                    res.body() = g_app->m_pages->portal_apps(*user);
                    conn->respond(res);
                    return;
                } else {
                    std::cerr <<"404 -- GET /portal : " <<path <<std::endl;
                    Connection::StringResponse res;
                    res.body() = "404 not found!\n";
                    res.result(404);
                    conn->respond(res);
                    return;
                }
            } else if (path == "/peer/key") {
                // Open the file
                // TODO would probably be better to use string body instead
                // TODO enable cache
                beast::error_code ec;
                boost::beast::http::file_body::value_type body;
                auto file_path = g_app->m_config.m_data_dir + "/auth/pubkey";
                body.open(file_path.c_str(), beast::file_mode::scan, ec);
                if (ec) {
                    conn->respond(server_error(ec.what()));
                    return;
                }

                // Send it
                Connection::FileResponse res{
                    boost::beast::http::status::ok,
                    conn->req().version(),
                    std::move(body)
                };
                conn->respond(res);
                return;
            } else {
                app_send_msg(std::move(conn));
                return;
            }

        case boost::beast::http::verb::post:
            if (path.starts_with("/portal")) {
                if (path.starts_with("/login")) {
                    login_post(std::move(conn));
                    return;
                } else if (path.starts_with("/signup")) {
                    signup_post(std::move(conn));
                    return;
                } else {
                    std::cerr <<"404 -- POST /portal : " <<path <<std::endl;
                    Connection::StringResponse res;
                    res.body() = "404 not found!\n";
                    res.result(404);
                    conn->respond(res);
                    return;
                }
            } else if (path == "/peer/handshake") {
                peer_handshake(std::move(conn));
                return;
            } else {
                app_send_msg(std::move(conn));
                return;
            }

        default:
            app_send_msg(std::move(conn));
    }
}