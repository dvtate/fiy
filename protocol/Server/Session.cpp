//
// Created by tate on 7/24/25.
//
#include "../util/WebUtils.hpp"
#include "../FIY.hpp"

#include "Router.hpp"
#include "Session.hpp"


void Session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    // FIXME why does this close the session?
    if (ec == boost::beast::http::error::end_of_stream) {
        DEBUG_LOG("end of stream, connection closed");
        return close();
    }

    if (ec) {
        std::cerr <<"HTTP Session Read failed: " << ec.message() <<'\n';
        auto bd = (char*) m_buffer.data().data();
        size_t i = 0;
        std::cout <<"Buffer: ";
        while (i < m_buffer.size()) {
            size_t incr = (i+10) > m_buffer.size() ? (m_buffer.size() - i) : 10;
            for (size_t j = 0; j < incr; j++) {
                std::cout <<"" <<bd[i+j];
            }
//            std::cout <<'\n';
            i+= incr;
        }
        std::cout <<std::endl;
        return;
    }

    route_request(shared_from_this());
}


std::map<std::string, std::string>& Session::get_cookies() {
    if (m_cookies.empty()) {
        auto cookie_header = m_req["Cookie"];
        if (!cookie_header.empty()) {
            m_cookies = WebUtils::parse_cookies(cookie_header);
//            std::cout << "Cookies: " << cookie_header << ": " << m_cookies.size() << std::endl;
        }
    }
    return m_cookies;
}

std::shared_ptr<LocalUser> Session::find_user_local() {
    // Try using cookies
    const auto& cookies = get_cookies();
    const auto token = cookies.find("fiy_auth");
    if (token != cookies.end())
        return g_fiy->m_users.auth_user(token->second);

    // Basic auth (used by git mod)
    const auto it = m_req.find("Authorization");
    if (it == m_req.end())
        return nullptr;

    // Get the encoded part
    auto v = it->value();
    if (!v.starts_with("Basic "))
        return nullptr;
    v.remove_prefix(6);

    // Decode base64
    namespace b64 = boost::beast::detail::base64;
    const auto p = std::make_unique<char[]>(b64::decoded_size(v.size()));
    b64::decode(p.get(), v.data(), v.size());

    // Split username+password
    std::string_view auth {p.get()};
    const auto i = auth.find(':');
    if (i == std::string_view::npos)
        return nullptr;

    // Check login
    auto ret = DB::get_user(
        std::string(auth.substr(0, i)),
        std::string(auth.substr(i + 1))
    );

    // Remove header so that we don't forward valid password anywhere else
    if (ret != nullptr)
        m_req.erase(it);

    return ret;
}

Session::User Session::find_user() {
    static const User unauthenticated = { .domain=nullptr, .user=" " };

    // Local User authentication
    if (const auto u = find_user_local(); u != nullptr)
        return { .domain=nullptr, .user=u->get_username() };

    // Peer authentication
    auto it = m_req.find("Fiy-Peer");
    if (it == m_req.end() || it->value().empty()) {
//    if (*it.empty()) {
//        std::cout << "missing auth token\n";
        return unauthenticated;
    }
    auto peer = g_fiy->m_peers.get_peer_from_token(it->value());
    if (!peer) {
        DEBUG_LOG("Invalid peer auth token: " << it->value());
        return unauthenticated;
    }
    auto user = m_req.at("Fiy-User");
    DEBUG_LOG("remote user authenticated\n");
    return { .domain=peer->m_domain, .user=user };
}
