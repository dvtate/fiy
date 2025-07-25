//
// Created by tate on 7/24/25.
//
#include "../util/Cookie.hpp"
#include "../App.hpp"

#include "Router.hpp"
#include "Session.hpp"


void Session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == boost::beast::http::error::end_of_stream)
        return close();
    if (ec) {
        std::cerr <<"HTTP Session Read failed: " << ec.message() <<'\n';
        return;
    }

    route_request(shared_from_this());
}


std::map<std::string, std::string>& Session::get_cookies() {
    if (m_cookies.empty()) {
        auto cookie_header = m_req["Cookie"];
        if (!cookie_header.empty()) {
            m_cookies = Cookie::parse(cookie_header);
            std::cout << "Cookies: " << cookie_header << ": " << m_cookies.size() << std::endl;
        }
    }
    return m_cookies;
}


Session::User Session::find_user() {
    static const User unauthenticated = { .domain=nullptr, .user=" " };
    auto& cookies = get_cookies();

    // Local User authentication
    auto user_auth_token = cookies.find("fiy_auth");
    if (user_auth_token != cookies.end()) {
        auto user = g_app->m_users.auth_user(user_auth_token->second);
        if (user == nullptr)
            return unauthenticated;
        else
            return { .domain=nullptr, .user=user->get_username() };
    }

    // Peer authentication
    auto it = m_req.find("Fiy-Peer");
    if (it == m_req.end() || it->value().empty()) {
//    if (*it.empty()) {
//        std::cout << "missing auth token\n";
        return unauthenticated;
    }
    auto peer = g_app->m_peers.get_peer_from_token(it->value());
    if (!peer) {
        DEBUG_LOG("Invalid peer auth token: " << it->value());
        return unauthenticated;
    }
    auto user = m_req.at("Fiy-User");
    DEBUG_LOG("remote user authenticated\n");
    return { .domain=peer->m_domain, .user=user };
}

std::shared_ptr<LocalUser> Session::find_user_local() {
    auto& cookies = get_cookies();
    if (cookies.empty()) {
        DEBUG_LOG("NO COOKIES?!");
    }
    auto user_auth_token = cookies.find("fiy_auth");
    if (user_auth_token != cookies.end())
        return g_app->m_users.auth_user(user_auth_token->second);
    return nullptr;
}
