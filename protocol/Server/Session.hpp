//
// Created by tate on 7/24/25.
//

#pragma once

#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/config.hpp>

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <map>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class LocalUser;


// Handles an HTTP server connection
class Session : public std::enable_shared_from_this<Session> {
    boost::beast::tcp_stream m_stream;
    boost::beast::flat_buffer m_buffer;
    boost::beast::http::request<boost::beast::http::string_body> m_req;

    std::map<std::string, std::string> m_cookies;

    using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

public:
    using Req = boost::beast::http::request<boost::beast::http::string_body>;
    using StringResponse = boost::beast::http::response<boost::beast::http::string_body>;
    using DynamicResponse = boost::beast::http::response<boost::beast::http::dynamic_body>;
    using FileResponse = boost::beast::http::response<boost::beast::http::file_body>;
    using EmptyResponse = boost::beast::http::response<boost::beast::http::empty_body>;
    using StringBody = boost::beast::http::string_body;
    using DynamicBody = boost::beast::http::dynamic_body;
    using FileBody = boost::beast::http::file_body;
    using EmptyBody = boost::beast::http::empty_body;

    struct User {
        const char* domain{nullptr};
        std::string user;
    };

    // Take ownership of the stream
    explicit Session(tcp::socket&& socket): m_stream(std::move(socket)) { }

    inline Req& req() { return m_req; };
    std::map<std::string, std::string>& get_cookies();
    void clear_cookie_cache() { m_cookies.clear(); }
    User find_user();
    std::shared_ptr<LocalUser> find_user_local();

    void run() {
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

    void respond(boost::beast::http::message_generator&& res) {
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

    template<class T>
    inline boost::beast::http::message_generator prep(boost::beast::http::response<T> msg) {
        msg.keep_alive(m_req.keep_alive());
        msg.prepare_payload();
        return boost::beast::http::message_generator(std::move(msg));
    }

    void close() {
        // Send a TCP shutdown
        beast::error_code ec;
        m_stream.socket().shutdown(tcp::socket::shutdown_send, ec);

        // At this point the connection is closed gracefully
    }

protected:
    void do_read() {
        // Make the request empty before reading,
        // otherwise the operation behavior is undefined.
        m_req = {};

        // Set the timeout.
        m_stream.expires_after(std::chrono::seconds(30));

        // Read a request
        boost::beast::http::async_read(m_stream, m_buffer, m_req,
             beast::bind_front_handler(
                 &Session::on_read,
                 shared_from_this()));
    }

    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);

    void on_write(
        bool keep_alive,
        beast::error_code ec,
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
};
