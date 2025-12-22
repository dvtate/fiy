//
// Created by tate on 7/24/25.
//
#include "Session.hpp"

#include <cstdlib>
#include <iostream>
#include <thread>

// #include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/config.hpp>

#include "../util/WebUtils.hpp"
#include "../FIY.hpp"

#include "Router.hpp"

void Session::run() {
    // We need to be executing within a strand to perform async operations
    // on the I/O objects in this session. Although not strictly necessary
    // for single-threaded contexts, this example code is written to be
    // thread-safe by default.
    boost::asio::dispatch(m_stream.get_executor(),
        boost::beast::bind_front_handler(
            &Session::do_read,
            shared_from_this()
        )
    );
}

void Session::respond(boost::beast::http::message_generator&& res) {
    bool keep_alive = res.keep_alive();

    // Write the response
    boost::beast::async_write(
        m_stream,
        std::move(res),
        boost::beast::bind_front_handler(
            &Session::on_write,
            shared_from_this(),
            keep_alive
        )
    );
}

void Session::close() {
    // Send a TCP shutdown
    boost::beast::error_code ec;
    m_stream.socket().shutdown(tcp::socket::shutdown_send, ec);
    if (ec) {
        DEBUG_LOG("Failed to shutdown socket connection: " << ec.message());
    }

    // At this point the connection is closed gracefully
}

void Session::do_read() {
    // Make the request empty before reading,
    // otherwise the operation behavior is undefined.

    m_req_parser.reset();
    m_req_parser.emplace(Req{});
    m_req_parser->body_limit(boost::none);

    // Read a request
    boost::beast::http::async_read(m_stream, m_buffer, m_req_parser.value(),
         boost::beast::bind_front_handler(
             &Session::on_read,
             shared_from_this()));
}

void Session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if (ec == boost::beast::http::error::end_of_stream) {
        // DEBUG_LOG("end of stream, connection closed");
        return close();
    }

    if (ec) {
        std::cerr <<"HTTP Session Read failed: " << ec.message() <<'\n';
#if 0
        auto bd = (char*) m_buffer.data().data();
        size_t i = 0;
        std::cout <<"Buffer: ";
        while (i < m_buffer.size()) {
            size_t incr = (i+10) > m_buffer.size() ? (m_buffer.size() - i) : 10;
            for (size_t j = 0; j < incr; j++) {
                if (isalnum(bd[i+j]))
                    std::cout <<"" <<bd[i+j];
                else
                    std::cout <<" " <<std::setw(2) << std::setfill('0') << std::hex << (int)( bd[i+j] ) <<" ";
            }
//            std::cout <<'\n';
            i+= incr;
        }
        std::cout <<std::endl;
#endif
        return;
    }

    route_request(shared_from_this());
}


void Session::on_write(
    bool keep_alive,
    boost::beast::error_code ec,
    std::size_t bytes_transferred
) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        std::cerr <<"HTTP Session Write failed: " <<ec.message() <<'\n';
        return;
    }

    // This means we should close the connection, usually because
    // the response indicated the "Connection: close" semantic.
    if (!keep_alive)
        return close();

    do_read();
}


std::map<std::string, std::string>& Session::get_cookies() {
    if (m_cookies.empty()) {
        const auto cookie_header = req()["Cookie"];
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
    const auto it = req().find("Authorization");
    if (it == req().end())
        return nullptr;
    DEBUG_LOG("Using http basic auth");

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
    std::string_view auth(p.get());
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
        req().erase(it);

    return ret;
}

Session::User Session::find_user() {
    static const User unauthenticated = { .domain=nullptr, .user=" " };

    // Local User authentication
    if (const auto u = find_user_local(); u != nullptr)
        return { .domain=nullptr, .user=u->get_username() };

    // Peer authentication
    const auto it = req().find("Fiy-Peer");
    if (it == req().end() || it->value().empty()) {
//    if (*it.empty()) {
//        std::cout << "missing auth token\n";
        return unauthenticated;
    }
    const auto peer = g_fiy->m_peers.get_peer_from_token(it->value());
    if (!peer) {
        DEBUG_LOG("Invalid peer auth token: " << it->value());
        return unauthenticated;
    }
    const auto user = req().at("Fiy-User");
    DEBUG_LOG("remote user authenticated\n");
    return { .domain=peer->m_domain, .user=user };
}
