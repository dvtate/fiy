//
// Created by tate on 6/25/25.
//

#include <boost/url.hpp>

#include "../util/WebUtils.hpp"

#include "../FIY.hpp"

#include "Session.hpp"
#include "Router.hpp"

static inline std::pair<std::string, std::string> parse_mod_request_get(const std::shared_ptr<Session>& conn) {
    auto path = conn->req().target();
    if (path.starts_with("/mods")) {
        path.remove_prefix(5);
        if (path[0] == '/')
            path.remove_prefix(1);
        return { path, conn->req().at("Fiy-Path") };
    }

    auto hostname = conn->req()["Host"];
    if (!hostname.empty()) {
        std::string_view hhn = g_fiy->m_config.m_hostname;
        // Subdomain mod  mod.example.com/uri/path
        if (hostname.ends_with(hhn) && hhn.size() != hostname.size())
            return {
                hostname.substr(0, hostname.size() - hhn.size() - 1),
                path
            };

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
            LOG_ERR(  "Received request for invalid hostname: expected "
                      << hhn.data()
                      << " but host was "
                      << hostname
                      << '\n');
            return { " ", "/" }; // give invalid mod
        }
    }

    // Not a subdomain mod  example.com/mod/uri/path
    const auto slash_idx = path.find('/', 1);
    if (slash_idx == std::string_view::npos)
        return { path.empty() ? "" : path.substr(1), "/" };
    else
        return { path.substr(1, slash_idx - 1), path.substr(slash_idx) };
}

static inline void mod_send_msg(std::shared_ptr<Session> conn) {
    // Get app
    auto [ mod, uri ] = parse_mod_request_get(conn);

    // Forward to mods
    Mod* m = g_fiy->m_mods.get_mod(mod);
    if (m == nullptr) {
        Session::DynamicResponse res;
        res.result(404);
        boost::beast::ostream(res.body())
                <<"App '" <<mod <<"' not found\n";
        res.set(boost::beast::http::field::content_type, "text/html");
        conn->respond(conn->prep(std::move(res)));
        DEBUG_LOG("Invalid mod: " <<mod <<" : " <<uri);
        return;
    }
    DEBUG_LOG("Calling App " <<mod <<" : " << uri);

    conn->req().target(uri);
    m->m_ipc->handle_request(std::move(conn));
}

// TODO ?redirect=/mod/that/requires/auth query parameter
static Session::StringResponse signup_page(unsigned status = 200, const std::string& err = "") {
    // Cache-able
    Session::StringResponse res;
    res.result(status);
    res.set(boost::beast::http::field::content_type, "text/html");
    res.body() = g_fiy->m_pages->signup_page(err);
    return res;
}
static Session::StringResponse login_page(unsigned status = 200, const std::string& err = "") {
    Session::StringResponse res;
    res.result(status);
    res.set(boost::beast::http::field::content_type, "text/html");
    res.body() = g_fiy->m_pages->login_page(err);
    return res;
}

/**
 * Checks if the proposed username string is allowable
 * @param str proposed username
 * @return true if valid, false otherwise
 */
static bool is_valid_username(const std::string& str) {
    for (const auto& c : str)
        if (! (isalnum(c) || c == '_' || c == '.'))
            return false;
    return true;
}

static void signup_post(std::shared_ptr<Session>&& conn) {
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
    if (!is_valid_username(username)) {
        conn->respond(conn->prep(resp_bad_username));
        return;
    }
    if (username.size() > LocalUser::USERNAME_MAX_LENGTH) {
        conn->respond(conn->prep(resp_long_username));
        return;
    }
    if (g_fiy->m_users.get_username(username) != nullptr) {
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
    res.result(boost::beast::http::status::see_other); // 303: post -> get
    // FIXME? https://stackoverflow.com/questions/18492576/share-cookies-between-subdomain-and-domain
    conn->clear_cookie_cache();
    res.set(boost::beast::http::field::set_cookie,
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
        res.set(boost::beast::http::field::location, (*it).value);
    else
        res.set(boost::beast::http::field::location, "/portal");
    conn->respond(conn->prep(std::move(res)));
}

static void login_post(std::shared_ptr<Session>&& conn) {
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
    auto auth_token = g_fiy->m_users.login_user(username, std::move(password));
    if (auth_token.m_user == nullptr) {
        conn->respond(conn->prep(resp_bad_creds));
        DEBUG_LOG("wrong username/password for user " <<username);
        return;
    }

    // Log user in and send them to portal home
    Session::EmptyResponse res;
    res.result(boost::beast::http::status::see_other); // 303 - redirect them with get request
    // FIXME? https://stackoverflow.com/questions/18492576/share-cookies-between-subdomain-and-domain
    conn->clear_cookie_cache();
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
    const auto ps = boost::urls::url_view(conn->req().target()).params();
    if (auto it = ps.find("redirect"); it != ps.end())
        res.set(boost::beast::http::field::location, (*it).value);
    else
        res.set(boost::beast::http::field::location, "/portal");
    conn->respond(conn->prep(std::move(res)));
}

template <class Str>
auto server_error(const Str& what) {
    Session::StringResponse res;
    res.result(boost::beast::http::status::internal_server_error);
    res.set(boost::beast::http::field::content_type, "text/html");
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
    ] (boost::beast::http::response<boost::beast::http::string_body> res) {
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

    boost::beast::http::request<boost::beast::http::empty_body> key_req;
    key_req.target("/peer/key");
    key_req.method(boost::beast::http::verb::get);
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
                        g_fiy->m_pages->file_contents<subpath>()
                    };
                    res.set(boost::beast::http::field::content_type, "text/javascript");
                    conn->respond(conn->prep(std::move(res)));
                    return;
                } else if (path.starts_with("/redirect")) {
                    // Check for redirect header
                    if (auto it = conn->req().find("FIY-Redirect");
                        it != conn->req().end()
                    ) {
                        Session::EmptyResponse res;
                        res.result(boost::beast::http::status::see_other); // 303 -> redirect as GET
                        res.set(boost::beast::http::field::location, it->value());
                        conn->respond(conn->prep(std::move(res)));
                        return;
                    }

                    // No redirect header, check for query params
                    const auto ps = boost::urls::url_view(conn->req().target()).params();
                    if (auto it = ps.find("redirect"); it != ps.end()) {
                        Session::EmptyResponse res;
                        res.result(boost::beast::http::status::see_other);
                        res.set(boost::beast::http::field::location, (*it).value);
                        conn->respond(conn->prep(std::move(res)));
                        return;
                    }

                    // Invalid request
                    Session::StringResponse res;
                    res.result(boost::beast::http::status::bad_request);
                    res.body() = "Invalid redirect: no destination provided";
                    conn->respond(conn->prep(std::move(res)));
                    return;
                } else if (path.empty() || path == "/") {
                    // Authenticate user
                    auto user = conn->find_user_local();
                    if (user == nullptr) {
                        DEBUG_LOG("User not logged in");
                        Session::EmptyResponse res;
                        res.result(boost::beast::http::status::see_other);
                        res.set(boost::beast::http::field::location, "/portal/login");
                        conn->respond(conn->prep(std::move(res)));
                        return;
                    }

                    // Send them to portal home
                    Session::StringResponse res;
                    res.result(200);
                    res.body() = g_fiy->m_pages->portal_apps(*user);
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
                // Send key
                Session::StringResponse res;
                res.result(200);
                res.body() = g_fiy->m_config.m_public_key;
                conn->respond(conn->prep(std::move(res)));
                return;
            } else {
                mod_send_msg(std::move(conn));
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
                mod_send_msg(std::move(conn));
                return;
            }

        default:
            mod_send_msg(std::move(conn));
            return;
    }
}