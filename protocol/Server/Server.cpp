//
// Created by tate on 6/25/25.
//

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>

#include "App.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

#include "Server.hpp"
#include "Connection.hpp"

// "Loop" forever accepting new connections.
static void http_server(tcp::acceptor& acceptor, tcp::socket& socket) {
    acceptor.async_accept(socket,
        [&](beast::error_code ec)
        {
          if(!ec)
              std::make_shared<Connection>(std::move(socket))->start();
          http_server(acceptor, socket);
        }
    );
}

void Server::start() {
    auto const address = net::ip::make_address("127.0.0.1");
    auto const port = static_cast<unsigned short>(g_app->m_config.m_port);

    tcp::acceptor acceptor{*g_app->m_ioc, {address, port}};
    tcp::socket socket{*g_app->m_ioc};
    http_server(acceptor, socket);
    g_app->m_ioc->run();
}