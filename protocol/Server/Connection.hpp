//
// Created by tate on 7/10/25.
//

#pragma once

#include <memory>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>

class LocalUser;

class Connection : public std::enable_shared_from_this<Connection> {
private:
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

    template <class T, typename... Args>
    inline boost::beast::http::response<T> Res(Args&& ...args) {
        auto ret = boost::beast::http::response<T>(std::forward<Args>(args)...);
        ret.version(m_request.version());
        ret.keep_alive(false);
        return ret;
    }

    struct User {
        const char* domain{nullptr};
        std::string user;
    };

    explicit Connection(tcp::socket socket);
    void start();

private:
    // The socket for the currently connected client.
    tcp::socket m_socket;

    // The buffer for performing reads.
    boost::beast::flat_buffer m_buffer{8192};

    // The request message.
    Req m_request;

    // The timer for putting a deadline on connection processing.
    boost::asio::steady_timer m_deadline {
        m_socket.get_executor(),
        std::chrono::seconds(60)
    };

    std::map<std::string, std::string> m_cookies;

public:
    Req& req() { return m_request; };

    /**
     * Asynchronously transmit the response message.
     */
    template<class T>
    void respond(boost::beast::http::response<T>& response) {
        auto self = shared_from_this();

        response.keep_alive(false);
        response.prepare_payload();

        boost::beast::http::async_write(
                m_socket,
                response,
                [self](boost::beast::error_code ec, std::size_t)
                {
                    self->m_socket.shutdown(tcp::socket::shutdown_send, ec);
                    self->m_deadline.cancel();
                }
        );
    }

    /**
     * Asynchronously transmit the response message.
     */
    template<class T>
    void respond(boost::beast::http::response<T>&& response) {
        auto self = shared_from_this();

        response.content_length(response.body().size());
        response.keep_alive(false);

        boost::beast::http::async_write(
                m_socket,
                response,
                [self](boost::beast::error_code ec, std::size_t)
                {
                    self->m_socket.shutdown(tcp::socket::shutdown_send, ec);
                    self->m_deadline.cancel();
                }
        );
    }

    std::map<std::string, std::string>& get_cookies();
    User find_user();
    std::shared_ptr<LocalUser> find_user_local();

private:
    void read_request();
    void timeout();
};

