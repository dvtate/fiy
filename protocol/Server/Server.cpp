//
// Created by tate on 6/25/25.
//

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>

#include "FIY.hpp"

#include "Server.hpp"
#include "Session.hpp"

namespace {
namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// Report a failure
void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

// Accepts incoming connections and launches the sessions
class Listener : public std::enable_shared_from_this<Listener> {
    net::io_context& m_ioc;
    tcp::acceptor m_acceptor;

public:
    Listener(net::io_context& ioc, tcp::endpoint&& endpoint)
        : m_ioc(ioc), m_acceptor(net::make_strand(ioc))
    {
        beast::error_code ec;

        // Open the acceptor
        m_acceptor.open(endpoint.protocol(), ec);
        if (ec) {
            fail(ec, "open");
            return;
        }

        // Allow address reuse
        m_acceptor.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        m_acceptor.bind(endpoint, ec);
        if (ec) {
            fail(ec, "bind");
            return;
        }

        // Start listening for connections
        m_acceptor.listen(net::socket_base::max_listen_connections, ec);
        if (ec) {
            fail(ec, "listen");
            return;
        }
    }

    // Start accepting incoming connections
    void run() {
        do_accept();
    }

private:
    void do_accept() {
        // The new connection gets its own strand
        m_acceptor.async_accept(
            net::make_strand(m_ioc),
            beast::bind_front_handler(
                &Listener::on_accept,
                shared_from_this()));
    }

    void on_accept(beast::error_code ec, tcp::socket socket) {
        if (ec) {
            fail(ec, "accept");
            return; // To avoid infinite loop
        } else {
            // Create the session and run it
            std::make_shared<Session>(std::move(socket))->run();
        }

        // Accept another connection
        do_accept();
    }
};

} // namespace {}


void Server::start() {
    // If hostname includes a colon it's being used for testing.
    // Otherwise, we expect to be behind a reverse proxy that adds SSL
    auto const address = net::ip::make_address(
        strchr(g_fiy->m_config.m_hostname, ':') != nullptr ? "127.0.0.1" : "0.0.0.0"
    );
    auto const port = static_cast<unsigned short>(g_fiy->m_config.m_port);

    // Start listening
    std::make_shared<Listener>(
        *g_fiy->m_ioc,
        tcp::endpoint{address, port}
    )->run();

    // App::start() runs the io_context
}