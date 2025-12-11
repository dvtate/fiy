//
// Created by tate on 7/24/25.
//

#pragma once

#include <algorithm>
#include <string>
#include <map>
#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

class LocalUser;

// Handles an HTTP server connection
class Session : public std::enable_shared_from_this<Session> {
    boost::beast::tcp_stream m_stream;
    boost::beast::flat_buffer m_buffer;
    std::optional<
        boost::beast::http::request_parser<
            boost::beast::http::string_body>> m_req_parser;

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

        [[nodiscard]] std::string str() const {
            std::string ret = user;
            if (domain != nullptr) {
                ret += '@';
                ret += domain;
            }
            return ret;
        }
    };

    // Take ownership of the stream
    explicit Session(tcp::socket&& socket):
        m_stream(std::move(socket)),
        m_req_parser(Req{})
    {
    }

    Req& req() { return m_req_parser->get(); };

    void clear_cookie_cache() { m_cookies.clear(); }
    std::map<std::string, std::string>& get_cookies();

    std::shared_ptr<LocalUser> find_user_local();
    User find_user();

    void run();

    template<class T>
    boost::beast::http::message_generator prep(boost::beast::http::response<T> msg) {
        msg.keep_alive(m_req_parser->keep_alive());
        msg.prepare_payload();
        return boost::beast::http::message_generator(std::move(msg));
    }

    void respond(boost::beast::http::message_generator&& res);
    void close();

protected:
    void do_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_write(bool keep_alive, boost::beast::error_code ec, std::size_t bytes_transferred);
};
