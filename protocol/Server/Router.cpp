//
// Created by tate on 6/25/25.
//

#include "../util/WebUtils.hpp"

#include "../App.hpp"

#include "Session.hpp"
#include "Router.hpp"

inline std::pair<std::string, std::string> parse_app_request_get(const std::shared_ptr<Session>& conn) {
    auto path = conn->req().target();
    if (path.starts_with("/mods")) {
        path.remove_prefix(5);
        if (path[0] == '/')
            path.remove_prefix(1);
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
//            Session::DynamicResponse res;
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
    if (slash_idx == std::string_view::npos)
        return { path.substr(1), "/" };
    else
        return { path.substr(1, slash_idx - 1), path.substr(slash_idx) };
}

inline static void app_send_msg(std::shared_ptr<Session> conn) {
    // Get app
    auto [ app, uri ] = parse_app_request_get(conn);

    // Forward to apps
    Mod* m = g_app->m_mods.get_mod(app);
    if (m == nullptr) {
        Session::DynamicResponse res;
        res.result(404);
        boost::beast::ostream(res.body())
                <<"App '" <<app <<"' not found\n";
        res.set(boost::beast::http::field::content_type, "text/html");
        conn->respond(conn->prep(std::move(res)));
        DEBUG_LOG("Invalid apps: " <<app <<" : " <<uri);
        return;
    }
    DEBUG_LOG("Calling App " <<app <<" : " << uri);

    conn->req().target(uri);
    m->m_ipc->handle_request(std::move(conn));
}

inline Session::StringResponse signup_page(unsigned status = 200, const std::string& err = "") {
    // Cache-able
    Session::StringResponse res;
    res.result(status);
    res.set(boost::beast::http::field::content_type, "text/html");
    res.body() = g_app->m_pages->signup_page(err);
    return res;
}
inline Session::StringResponse login_page(unsigned status = 200, const std::string& err = "") {
    Session::StringResponse res;
    res.result(status);
    res.set(boost::beast::http::field::content_type, "text/html");
    res.body() = g_app->m_pages->login_page(err);
    return res;
}

inline bool str_is_alphanum(const std::string& str) {
    // c++20: std::ranges::all_of(str.cbegin(), str.cend(), isalnum);
    for (const auto& c : str)
        if (!isalnum(c))
            return false;
    return true;
}

void signup_post(std::shared_ptr<Session>&& conn) {
    static const auto resp_bad_username = signup_page(400, "Username should contain only alphanumeric characters");
    static const auto resp_username_taken = signup_page(400, "Username is taken, try a different one");
    static const auto resp_bad_form = signup_page(400, "Form Error");
    static const auto resp_long_username = signup_page(400, "Username must be less than " + std::to_string(LocalUser::USERNAME_MAX_LENGTH) +  " characters");
    static const auto resp_long_name = signup_page(400, "Name must be less than " + std::to_string(LocalUser::NAME_MAX_LENGTH) +  " characters");
    static const auto resp_long_contact = signup_page(400, "Contact info must be less than " + std::to_string(LocalUser::EMAIL_MAX_LENGTH) + " characters");
    static const auto resp_server_error = signup_page(500, "Failed to create account, try again later");

    // Parse form body
    std::deque<std::pair<std::string, std::string>> form;
    if (!WebUtils::parse_form_url_encoded(conn->req().body(), form)) {
        conn->respond(conn->prep(resp_bad_form));
        DEBUG_LOG("invalid body: '" <<conn->req().body() <<"'");
        return;
    }
    std::string username, password, contact, name;
    for (auto&& [k, v] : std::move(form)) {
        if (k == "username")
            username = std::move(v);
        else if (k == "password")
            password = std::move(v);
        else if (k == "contact")
            contact = std::move(v);
        else if (k == "name")
            name = std::move(v);
        else
            DEBUG_LOG("invalid form field: " <<k);
    }
    LOG("Creating local user: " << username <<"  | " << contact);

    // Validate username
    if (!str_is_alphanum(username)) {
        conn->respond(conn->prep(resp_bad_username));
        return;
    }
    if (username.size() > LocalUser::USERNAME_MAX_LENGTH) {
        conn->respond(conn->prep(resp_long_username));
        return;
    }
    if (g_app->m_users.get_username(username) != nullptr) {
        conn->respond(conn->prep(resp_username_taken));
        return;
    }
    if (name.size() > LocalUser::NAME_MAX_LENGTH) {
        conn->respond(conn->prep(resp_long_name));
        return;
    }

    // Validate contact
    if (contact.size() > LocalUser::EMAIL_MAX_LENGTH) {
        conn->respond(conn->prep(resp_long_contact));
        return;
    }

    // Create user
    LocalUser user{username, false, username, contact, "en", g_app->now()};
    try {
        if (!DB::add_user(user, password)) {
            LOG_ERR("Failed to create user?");
            conn->respond(conn->prep(resp_server_error));
            return;
        }
    } catch (const DB::Exception& e) {
        LOG_ERR("Failed to create new user: Database Error: " <<e.what());
    }

    auto auth_token = g_app->m_users.login_user(username, password);

    Session::EmptyResponse res;
    res.result(boost::beast::http::status::temporary_redirect);
    res.set(boost::beast::http::field::set_cookie,
        WebUtils::serialize_cookie(
            "fiy_auth",
            auth_token.m_token,
            {   .encode_fn=nullptr,
                .max_age=LocalUser::AuthToken::SESSION_LIFETIME,
                .path="/",
                .http_only=true
            }
        )
    );
    res.set(boost::beast::http::field::location, "/portal");
    conn->clear_cookie_cache();
    conn->respond(conn->prep(std::move(res)));
}

void login_post(std::shared_ptr<Session>&& conn) {
    // Pre-allocated responses
    static const auto resp_bad_form = login_page(400, "Form Error");
    static const auto resp_bad_creds = login_page(401, "Incorrect username/password");

    // Get username and password from body
    std::deque<std::pair<std::string, std::string>> form;
    if (!WebUtils::parse_form_url_encoded(conn->req().body(), form)) {
        conn->respond(conn->prep(resp_bad_form));
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
    auto auth_token = g_app->m_users.login_user(username, std::move(password));
    if (auth_token.m_user == nullptr) {
        conn->respond(conn->prep(resp_bad_creds));
        DEBUG_LOG("wrong username/password for user " <<username);
        return;
    }

    // Log user in and send them to portal home
    Session::EmptyResponse res;
    res.result(boost::beast::http::status::see_other); // 303 - redirect them with get request
    res.set(boost::beast::http::field::location, "/portal");
    res.set(boost::beast::http::field::set_cookie,
        WebUtils::serialize_cookie(
            "fiy_auth",
            auth_token.m_token,
            {   .encode_fn=nullptr,
                .max_age=LocalUser::AuthToken::SESSION_LIFETIME,
                .path="/",
                .http_only=true
            }
        )
    );
    conn->clear_cookie_cache();
    conn->respond(conn->prep(std::move(res)));
}

template <class Str>
auto server_error(const Str& what) {
    Session::StringResponse res;
    res.result(http::status::internal_server_error);
    res.set(http::field::content_type, "text/html");
    res.body() = "An error occurred: '" + std::string(what) + "'";
    return res;
}

std::vector<std::string> split_string(
    const std::string_view str,
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

void peer_handshake(std::shared_ptr<Session>&& conn) {
    // TODO this algorithm is insecure and just used for testing
    DEBUG_LOG("handshake: body: " << conn->req().body());
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
    Session::StringResponse res;
    res.result(200);
    res.body() = p->m_auth.m_bearer_token_we_accept;
    conn->respond(conn->prep(std::move(res)));

    // Actual algorithm to use:
    // 1. Use private key associated with our pubkey endpoint to decrypt request body
    // 2. Store provided peer data: host, symkey, tokens, etc. into Peers cache
    // 3. Send token + symkey, encrypted with the other peer's pubkey

    // Decrypt body
//    auto encryptedBody = drogon::utils::base64Decode(req->body()); // may contain \0

    // Get relevant public key
//    auto client = drogon::HttpsClient::newHttpClient(remote_peer);
//    client.request();

}

void route_request(std::shared_ptr<Session> conn) {
    auto path = conn->req().target();

    // Logging
    DEBUG_LOG("Req: "
        <<boost::beast::http::to_string(conn->req().method())
        <<" -- "
        <<path);

    switch (conn->req().method()) {
        case boost::beast::http::verb::get:
            if (path.starts_with("/portal")) {
                path.remove_prefix(7);
                if (path.starts_with("/login")) {
                    conn->respond(conn->prep(login_page()));
                    return;
                } else if (path.starts_with("/signup")) {
                    conn->respond(conn->prep(signup_page()));
                    return;
                } else if (path.starts_with("/main.js")) {
                    // Send cached file contents
                    static const char subpath[] = "/main.js";
                    Session::StringResponse res{
                        boost::beast::http::status::ok,
                        conn->req().version(),
                        g_app->m_pages->file_contents<subpath>()
                    };
                    res.set(boost::beast::http::field::content_type, "text/javascript");
                    conn->respond(conn->prep(std::move(res)));
                    return;
                } else if (path.empty() || path == "/") {
                    // Authenticate user
                    auto user = conn->find_user_local();
                    if (user == nullptr) {
                        DEBUG_LOG("User not logged in");
                        Session::EmptyResponse res;
                        res.result(boost::beast::http::status::temporary_redirect);
                        res.set(boost::beast::http::field::location, "/portal/login");
                        conn->respond(conn->prep(std::move(res)));
                        return;
                    }

                    // Send them to portal home
                    Session::StringResponse res;
                    res.result(200);
                    res.body() = g_app->m_pages->portal_apps(*user);
                    res.set(boost::beast::http::field::content_type, "text/html");
                    conn->respond(conn->prep(std::move(res)));
                    return;
                } else {
                    std::cerr <<"404 -- GET /portal : " <<path <<std::endl;
                    Session::StringResponse res;
                    res.body() = "404 not found!\n";
                    res.result(404);
                    conn->respond(conn->prep(std::move(res)));
                    DEBUG_LOG("404 - " <<conn->req().target());
                    return;
                }
            } else if (path == "/peer/key") {
                // Cache file contents
                static const std::string contents = FileCache::load_file_as_string(
                    g_app->m_config.m_data_dir + "/auth/key"
                );

                // Send contents
                Session::StringResponse res;
                res.result(200);
                res.body() = contents;
                conn->respond(conn->prep(std::move(res)));
                return;
            } else {
                app_send_msg(std::move(conn));
                return;
            }

        case boost::beast::http::verb::post:
            if (path.starts_with("/portal")) {
                path.remove_prefix(7);
                if (path.starts_with("/login")) {
                    login_post(std::move(conn));
                    return;
                } else if (path.starts_with("/signup")) {
                    signup_post(std::move(conn));
                    return;
                } else {
                    std::cerr <<"404 -- POST /portal : " <<path <<std::endl;
                    Session::StringResponse res;
                    res.body() = "404 not found!\n";
                    res.result(404);
                    conn->respond(conn->prep(std::move(res)));
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
            return;
    }
}