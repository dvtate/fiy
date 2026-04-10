//
// Created by tate on 6/25/25.
//

// TODO shouldn't need this
#include <boost/url.hpp>

#include "../util/WebUtils.hpp"

#include "../FIY.hpp"

#include "Session.hpp"
#include "Router.hpp"
#include "Pages.hpp"

namespace http = boost::beast::http;

/**
 * @return [ mod , subpath ]
 */
static std::pair<Mod*, std::string> parse_mod_request_get(const std::shared_ptr<Session>& conn) {
    // Mod-by-ID API
    auto path = conn->req().target();
    if (path.starts_with("/mods/")) {
        path.remove_prefix(6);
        auto mod_end = path.find_first_of("/?#");
        Mod* mod = g_fiy->m_mods.get_mod_by_id(
            path.substr(0, mod_end));
        std::string subpath = conn->req()["Fiy-Path"];
        if (subpath.empty() && mod_end != std::string_view::npos)
            subpath = path.substr(mod_end);
        if (subpath.empty())
            subpath = "/";
        return { mod, subpath };
    }

    // Subdomains
    const auto hostname = conn->req()["Host"];
    if (!hostname.empty()) {
        const std::string_view hhn = g_fiy->m_config.m_hostname;
        // Subdomain mod  mod.example.com/uri/path
        if (hostname.ends_with(hhn) && hhn.size() != hostname.size()) {
            const auto mod_name = hostname.substr(0, hostname.size() - hhn.size() - 1);
            Mod* mod = g_fiy->m_mods.get_mod(mod_name);
            return { mod, path };
        }

        // Invalid request to different host?
        if (hostname != hhn) {
//            Session::DynamicResponse res;
//            res.result(400);
//            boost::beast::ostream(res.body())
//                    << "Wrong host? Expected host to be "
//                    << hhn.data()
//                    << " but host was "
//                    << hostname
//                    << '\n';
//            conn->respond(res);
            LOG_ERR("Received request for invalid hostname: expected "
                << hhn.data()
                << " but host was "
                << hostname
                << '\n');
            return { nullptr, "/" }; // give invalid mod
        }
    }

    // NOTE: weird feature: Root Mods (ie - mod w/ path '')
    // this means that any time the mod path doesn't match
    // we need to assume it's a request for the root mod
    // so for example a request to /index.html would first check
    // for mod 'index.html' and if that doesn't exist it would try
    // mod '' with path '/index.html'

    // Not a subdomain mod
    const auto slash_idx = path.find('/', 1);
    if (slash_idx == std::string_view::npos) {
        // case: /mod
        // case: /
        const auto qs_idx = path.find('?', 1);
        if (qs_idx == std::string_view::npos) {
            Mod* mod = path.empty()
                ? g_fiy->m_mods.get_mod("")
                : g_fiy->m_mods.get_mod(path.substr(1));
            return { mod, "/" };
        }

        // case: /mod?param
        // case: /?param
        std::string sub_path = "/";
        sub_path += path.substr(qs_idx);
        Mod* mod = path.empty()
            ? g_fiy->m_mods.get_mod("")
            : g_fiy->m_mods.get_mod(path.substr(1, qs_idx - 1));
        return { mod, sub_path };
    }

    // case: /mod/
    // case: /mod/uri/path
    // case: /mod/uri/path/
    // case: /mod/?param
    // case: /mod/uri/path?param
    // case: /mod/uri/path/?param
    return {
        g_fiy->m_mods.get_mod(path.substr(1, slash_idx - 1)),
        path.substr(slash_idx)
    };
}

/**
 * Invoke app
 * @param conn connection
 */
static void mod_send_msg(std::shared_ptr<Session> conn) {
    // Get app
    auto [ m, uri ] = parse_mod_request_get(conn);

    // Forward to mods
    if (m == nullptr) {
        // No '' mod, use default index.html
        if (conn->req().target() == "/"
            && conn->req()["host"] == g_fiy->m_config.m_hostname
        ) {
            static const char subpath[] = "/index.html";
            Session::StringResponse res{
                http::status::ok,
                conn->req().version(),
                Pages::file_contents<subpath>()
            };
            res.set(http::field::content_type, "text/html");
            conn->respond(conn->prep(std::move(res)));
            return;
        }

        // Warn about missing mod
        Session::DynamicResponse res;
        res.result(404);
        boost::beast::ostream(res.body())
            << "<h1>404 - Could not find a mod to handle your request</h1>"
                "\n<hr/>\n<p>For a list of apps: check the <a href=\""
            << g_fiy->base_uri() << "/portal\">portal</a></p>\n";
        res.set(http::field::content_type, "text/html");
        conn->respond(conn->prep(std::move(res)));
        DEBUG_LOG("Invalid mod: " <<conn->req().target() <<" : " <<uri);
        return;
    }
    DEBUG_LOG("Calling App " <<m->m_id <<" : " << uri);

    conn->req().target(uri);
    m->m_ipc->handle_request(std::move(conn));
}

// TODO ?redirect=/mod/that/requires/auth query parameter
static void signup_post(std::shared_ptr<Session>&& conn) {
    // Cache prepared responses
    static const auto resp_bad_username
        = Pages::signup_page(400, "Username should contain only alphanumeric characters");
    static const auto resp_username_taken
        = Pages::signup_page(400, "Username is taken, try a different one");
    static const auto resp_username_reserved
        = Pages::signup_page(400, "Username is reserved, try a different one");
    static const auto resp_bad_form
        = Pages::signup_page(400, "Form Error");
    static const auto resp_long_username
        = Pages::signup_page(400, "Username must be less than " + std::to_string(LocalUser::USERNAME_MAX_LENGTH) +  " characters");
    static const auto resp_long_name
        = Pages::signup_page(400, "Name must be less than " + std::to_string(LocalUser::NAME_MAX_LENGTH) +  " characters");
    static const auto resp_long_contact
        = Pages::signup_page(400, "Contact info must be less than " + std::to_string(LocalUser::EMAIL_MAX_LENGTH) + " characters");
    static const auto resp_server_error
        = Pages::signup_page(500, "Failed to create account, try again later");

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
    for (const auto& c : username)
        if (! (isalnum(c) || c == '_' || c == '.')) {
            conn->respond(conn->prep(resp_bad_username));
            return;
        }
    if (username.size() > LocalUser::USERNAME_MAX_LENGTH || username.empty()) {
        conn->respond(conn->prep(resp_long_username));
        return;
    }
    static const std::unordered_set<std::string_view> reserved_usernames = {
        "api", "portal", "mod", "app", "help", "home", "index", "new",
    };
    if (reserved_usernames.contains(username) || username.ends_with("settings")) {
        conn->respond(conn->prep(resp_username_reserved));
        return;
    }
    if (g_fiy->m_users.get_username(username) != nullptr) {
        conn->respond(conn->prep(resp_username_taken));
        return;
    }

    // Validate name
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
    LocalUser user{username, false, username, contact, "en", g_fiy->now()};
    try {
        if (!DB::add_user(user, password)) {
            LOG_ERR("Failed to create user?");
            conn->respond(conn->prep(resp_server_error));
            return;
        }
    } catch (const DB::Exception& e) {
        LOG_ERR("Failed to create new user: Database Error: " <<e.what());
    }

    auto auth_token = g_fiy->m_users.login_user(username, password);

    // Find redirect query parameter
    Session::EmptyResponse res;
    res.result(http::status::see_other); // 303: post -> get
    // FIXME? https://stackoverflow.com/questions/18492576/share-cookies-between-subdomain-and-domain
    conn->clear_cookie_cache();
    res.set(http::field::set_cookie,
        WebUtils::serialize_cookie(
            "fiy_auth",
            auth_token.m_token,
            {
                .encode_fn=nullptr,
                .max_age=LocalUser::AuthToken::SESSION_LIFETIME,
                .path="/",
                .http_only=true
            }
        )
    );

    const auto ps = boost::urls::url_view(conn->req().target()).params();
    if (auto it = ps.find("redirect"); it != ps.end())
        res.set(http::field::location, (*it).value);
    else
        res.set(http::field::location, "/portal");
    conn->respond(conn->prep(std::move(res)));
}

static void login_post(std::shared_ptr<Session>&& conn) {
    // Pre-allocated responses
    static const auto resp_bad_form = Pages::login_page(400, "Form Error");
    static const auto resp_bad_creds = Pages::login_page(401, "Incorrect username/password");

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
    auto auth_token = g_fiy->m_users.login_user(username, std::move(password));
    if (auth_token.m_user == nullptr) {
        conn->respond(conn->prep(resp_bad_creds));
        DEBUG_LOG("wrong username/password for user " <<username);
        return;
    }

    // Log user in and send them to portal home
    Session::EmptyResponse res;
    res.result(http::status::see_other); // 303 - redirect them with get request
    // FIXME? https://stackoverflow.com/questions/18492576/share-cookies-between-subdomain-and-domain
    conn->clear_cookie_cache();
    res.set(http::field::set_cookie,
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
    const auto ps = boost::urls::url_view(conn->req().target()).params();
    if (auto it = ps.find("redirect"); it != ps.end())
        res.set(http::field::location, (*it).value);
    else
        res.set(http::field::location, "/portal");
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
    // DEBUG_LOG("handshake: body: " << conn->req().body());
    // auto body = Crypto::SSL::decrypt(
    //     g_fiy->m_config.m_private_key,
    //     conn->req().body()
    // );
    const auto& body = conn->req().body();

    // DEBUG_LOG("Incoming Handshake request:\n" << body << "\n");

    // Get body components
    // Body should contain the following:
    //  - peer domain
    //  - part of a secret
    //  - bearer token
    //  - signature
    std::string domain, secret, bearer_token, signature;
    auto eol = body.find('\n');
    if (eol != std::string::npos) {
        domain = body.substr(0, eol);
        size_t start = eol + 1;
        eol = body.find('\n', start);
        if (eol != std::string::npos) {
            secret = body.substr(start, eol - start);
            start = eol + 1;
            eol = body.find('\n', start);
            if (eol != std::string::npos) {
                bearer_token = body.substr(start, eol - start);
                signature = body.substr(eol + 1);
            }
        }
    }

    // Missing components
    if (signature.empty() || bearer_token.empty() || secret.empty() || domain.empty()) {
        Session::StringResponse res;
        res.result(400);
        res.body() = "Invalid handshake body: missing components";
        conn->respond(conn->prep(std::move(res)));
        return;
    }

    // TODO is there a reason/process for re-authentication
    if (g_fiy->m_peers.get_peer_for_domain(domain) != nullptr) {
        Session::StringResponse res;
        res.result(400);
        res.body() = "Already peers";
        conn->respond(conn->prep(std::move(res)));
        return;
    }

    // Get foreign public key
    auto with_pubkey_cb = [
        domain,
        secret = std::move(secret),
        our_token = std::move(bearer_token),
        handshake_sig = std::move(signature),
        conn = std::move(conn)
    ] (http::response<http::string_body> res) {
        // Make sure it's success
        if (res.result_int() != 200) {
            Session::StringResponse ret;
            ret.result(ret.result());
            ret.body() = "Failed to get public key from domain " + domain;
            conn->respond(conn->prep(std::move(ret)));
            LOG_ERR("Failed to get public key for handshake peer "
                <<domain <<": " <<ret.result_int());
        }

        // Verify signature
        const auto pubkey = res.body();
        const std::string sig_data = domain + '\n' + secret + '\n' + our_token;
        if (!Crypto::SSL::verify(pubkey, sig_data, handshake_sig)) {
            Session::StringResponse ret;
            ret.result(401);
            ret.body() = "Invalid signature";
            conn->respond(conn->prep(std::move(ret)));
            DEBUG_LOG("Invalid signature received in handshake");
            return;
        }

        // Our contribution to the secret
        std::string our_secret = PeerAuth::get_token_string();

        // Add peer to the db
        auto peer = std::make_shared<Peer>(
            domain,
            PeerAuth{
                secret + our_secret, our_token
            }
        );

        int max_tries = 10;
        while (!g_fiy->m_peers.add_peer(domain, peer) && max_tries-- > 0) {
            // Maybe another thread already completed the handshake?
            if (g_fiy->m_peers.get_peer_for_domain(domain) != nullptr) {
                Session::StringResponse ret;
                ret.result(400);
                ret.body() = "Already peers";
                conn->respond(conn->prep(std::move(ret)));
                return;
            }

            // Update token and try again
            peer->m_auth.m_bearer_token_we_accept = PeerAuth::get_token_string();
        }

        // Respond with
        //  - bearer token
        //  - our part of the secret
        //  - signature
        std::string response = peer->m_auth.m_bearer_token_we_accept
            + '\n' + our_secret;
        auto sig = Crypto::SSL::sign(g_fiy->m_config.m_private_key, response);
        response += '\n' + sig;

        Session::StringResponse ret;
        ret.result(200);
        // TODO only encrypt the secret and bearer token
        // ret.body() = Crypto::SSL::encrypt(pubkey, response);
        ret.body() = response;
        conn->respond(conn->prep(std::move(ret)));
    };

    auto err_key_cb = [domain, conn = std::move(conn)](const std::string& message) {
        Session::StringResponse res;
        res.result(404);
        res.body() = "Could not get public key from domain " + domain;
        conn->respond(conn->prep(std::move(res)));
        LOG_ERR("Could not get public key for handshake peer " <<domain <<": " <<message);
    };

    http::request<http::empty_body> key_req;
    key_req.target("/peer/key");
    key_req.method(http::verb::get);
    // key_req.keep_alive(false);

    if (domain.find(':') == std::string_view::npos) {
        g_fiy->m_https.request(
            domain,
            std::move(key_req),
            std::move(with_pubkey_cb),
            std::move(err_key_cb)
        );
    } else {
        g_fiy->m_http.request(
            domain,
            std::move(key_req),
            std::move(with_pubkey_cb),
            std::move(err_key_cb)
        );
    }
}

void login_user_internal(const std::shared_ptr<Session>& conn) {
    const auto token = conn->req()["fiy-auth"];
    const auto username = conn->req()["fiy-user"];
    const auto password = conn->req()["fiy-pass"];

    if (! g_fiy->m_mods.has_net_connector(token)) {
        http::response<http::empty_body> res;
        res.result(401);
        conn->respond(conn->prep(std::move(res)));
        return;
    }

    try {
        auto u = DB::get_user(username, password);
        http::response<http::empty_body> res;
        res.result(u == nullptr ? 401 : 200);
        conn->respond(conn->prep(std::move(res)));
        return;
    } catch (const DB::Exception& e) {
        LOG_ERR("DB Error: " <<e.what());
        http::response<http::string_body> res;
        res.result(500);
        res.body() = "Database Error";
        conn->respond(conn->prep(std::move(res)));
    }
}

void route_request(std::shared_ptr<Session> conn) {
    auto path = conn->req().target();

    // Logging
    DEBUG_LOG("Req: "
        <<http::to_string(conn->req().method())
        <<" -- "
        <<path);

    switch (conn->req().method()) {
        case http::verb::get:
            if (path.starts_with("/portal")) {
                path.remove_prefix(7);
                if (path.starts_with("/login")) {
                    conn->respond(conn->prep(Pages::login_page()));
                    return;
                } else if (path.starts_with("/signup")) {
                    conn->respond(conn->prep(Pages::signup_page()));
                    return;
                } else if (path.starts_with("/theme.js")) {
                    // Send cached file contents
                    static const char subpath[] = "/theme.js";
                    Session::StringResponse res{
                        http::status::ok,
                        conn->req().version(),
                        Pages::file_contents<subpath>()
                    };
                    res.set(http::field::content_type, "text/javascript");
                    conn->respond(conn->prep(std::move(res)));
                    return;
                } else if (path.starts_with("/main.css")) {
                    // Send cached file contents
                    static const char subpath[] = "/assets/minstyle.io.min.css";
                    Session::StringResponse res{
                        http::status::ok,
                        conn->req().version(),
                        Pages::file_contents<subpath>()
                    };
                    res.set(http::field::content_type, "text/css");
                    conn->respond(conn->prep(std::move(res)));
                    return;
                } else if (path.starts_with("/redirect")) {
                    // Check for redirect header
                    if (auto it = conn->req().find("FIY-Redirect");
                        it != conn->req().end()
                    ) {
                        Session::EmptyResponse res;
                        res.result(http::status::see_other); // 303 -> redirect as GET
                        res.set(http::field::location, it->value());
                        conn->respond(conn->prep(std::move(res)));
                        return;
                    }

                    // No redirect header, check for query params
                    const auto ps = boost::urls::url_view(conn->req().target()).params();
                    if (auto it = ps.find("redirect"); it != ps.end()) {
                        Session::EmptyResponse res;
                        res.result(http::status::see_other);
                        res.set(http::field::location, (*it).value);
                        conn->respond(conn->prep(std::move(res)));
                        return;
                    }

                    // Invalid request
                    Session::StringResponse res;
                    res.result(http::status::bad_request);
                    res.body() = "Invalid redirect: no destination provided";
                    conn->respond(conn->prep(std::move(res)));
                    return;
                } else if (path.empty() || path == "/") {
                    // Authenticate user
                    auto user = conn->find_user_local();
                    if (user == nullptr) {
                        DEBUG_LOG("User not logged in");
                        Session::EmptyResponse res;
                        res.result(http::status::see_other);
                        res.set(http::field::location, "/portal/login");
                        conn->respond(conn->prep(std::move(res)));
                        return;
                    }

                    // Send them to portal home
                    conn->respond(conn->prep(Pages::portal_apps(*user)));
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
                // Send key
                Session::StringResponse res;
                res.result(200);
                res.set("Fiy-Now", std::to_string(g_fiy->now()));
                res.body() = g_fiy->m_config.m_public_key;
                conn->respond(conn->prep(std::move(res)));
                return;
            } else if (path == "/robots.txt") {
                Session::StringResponse res;
                res.result(200);
                static constexpr char robots_txt[] = "/robots.txt";
                res.body() = Pages::file_contents<robots_txt>();
                res.set(http::field::content_type, "text/plain");
                res.set("Cache-control", "max-age=60000");
                conn->respond(conn->prep(std::move(res)));
                return;
            } else {
                mod_send_msg(std::move(conn));
                return;
            }

        case http::verb::post:
            if (path.starts_with("/portal")) {
                path.remove_prefix(7);
                if (path.starts_with("/login")) {
                    // TODO ?redirect=/mod/that/requires/auth query parameter
                    login_post(std::move(conn));
                    return;
                } else if (path.starts_with("/signup")) {
                    // TODO ?redirect=/mod/that/requires/auth query parameter
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
                mod_send_msg(std::move(conn));
                return;
            }

        default:
            mod_send_msg(std::move(conn));
            return;
    }
}