//
// Created by tate on 7/10/25.
//

#include "../util/Cookie.hpp"
#include "../App.hpp"

#include "Router.hpp"

#include "Connection.hpp"

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


Connection::Connection(tcp::socket socket)
    : m_socket(std::move(socket))
    {}

/**
 * Initiate async operations for this connection
 */
void Connection::start() {
    read_request();
    timeout();
}

/**
 *  Asynchronously receive a complete request message.
 */
void Connection::read_request() {
    auto self = shared_from_this();

    http::async_read(
            m_socket,
            m_buffer,
            m_request,
            [self](beast::error_code ec,
                   std::size_t bytes_transferred)
            {
                boost::ignore_unused(bytes_transferred);
                if(!ec)
                    route_request(self);
            });
}


void Connection::timeout() {
    auto self = shared_from_this();

    m_deadline.async_wait(
        [self](beast::error_code ec) {
            if (!ec) {
                // Close socket to cancel any outstanding operation.
                self->m_socket.close(ec);
            }
        }
    );
}

std::map<std::string, std::string>& Connection::get_cookies() {
    if (m_cookies.empty()) {
        auto cookie_header = m_request.at("Cookie");
        if (!cookie_header.empty())
            m_cookies = Cookie::parse(cookie_header);
    }
    return m_cookies;
}

Connection::User Connection::find_user() {
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
    auto peer_token = m_request.at("Fiy-Peer");
    if (peer_token.empty()) {
//        std::cout << "missing auth token\n";
        return unauthenticated;
    }
    auto peer = g_app->m_peers.get_peer_from_token(peer_token);
    if (!peer) {
        DEBUG_LOG("Invalid peer auth token: " << peer_token);
        return unauthenticated;
    }
    auto user = m_request.at("Fiy-User");
    DEBUG_LOG("remote user authenticated\n");
    return { .domain=peer->m_domain, .user=user };
}

std::shared_ptr<LocalUser> Connection::find_user_local() {
    auto& cookies = get_cookies();
    auto user_auth_token = cookies.find("fiy_auth");
    if (user_auth_token != cookies.end())
        return g_app->m_users.auth_user(user_auth_token->second);
    return nullptr;
}

//void Connection::set_cookie(const std::string& name, const std::string& value, Cookie::CookieOptions options) {
//    m_response.set("Set-Cookie", Cookie::serialize(name, value, options));
//}
//void Connection::set_cookie(const std::string& name, const std::string& value) {
//    m_response.set("Set-Cookie", Cookie::serialize(name, value));
//}

/*
class Connection : public std::enable_shared_from_this<Connection>
{
public:


public:
    explicit Connection(tcp::socket socket)
            : m_socket(std::move(socket))
    {}

    // Initiate the asynchronous operations associated with the connection.
    void start() {
        read_request();
        check_deadline();
    }

private:
    // The socket for the currently connected client.
    tcp::socket m_socket;

    // The buffer for performing reads.
    beast::flat_buffer m_buffer{8192};

    // The request message.
    http::request<http::dynamic_body> m_request;

    // The response message.
    http::response<http::dynamic_body> m_response;

    // The timer for putting a deadline on connection processing.
    net::steady_timer m_deadline {
            m_socket.get_executor(), std::chrono::seconds(60)};

    //

    // Determine what needs to be done with the request message.
    void process_request() {
        m_response.version(m_request.version());
        m_response.keep_alive(false);

        switch(m_request.method()) {
            case http::verb::get:
                m_response.result(http::status::ok);
                m_response.set(http::field::server, "Beast");
                create_response();
                break;

            default:
                // We return responses indicating an error if
                // we do not recognize the request method.
                m_response.result(http::status::bad_request);
                m_response.set(http::field::content_type, "text/plain");
                beast::ostream(m_response.body())
                        << "Invalid request-method '"
                        << std::string(m_request.method_string())
                        << "'";
                break;
        }

        write_response();
    }

    // Construct a response message based on the program state.
    void create_response() {
        if(m_request.target() == "/count") {
            m_response.set(http::field::content_type, "text/html");
            beast::ostream(m_response.body())
                    << "<html>\n"
                    <<  "<head><title>Request count</title></head>\n"
                    <<  "<body>\n"
                    <<  "<h1>Request count</h1>\n"
                    <<  "<p>There have been "
                    <<  my_program_state::request_count()
                    <<  " requests so far.</p>\n"
                    <<  "</body>\n"
                    <<  "</html>\n";
        } else if(m_request.target() == "/time") {
            m_response.set(http::field::content_type, "text/html");
            beast::ostream(m_response.body())
                    <<  "<html>\n"
                    <<  "<head><title>Current time</title></head>\n"
                    <<  "<body>\n"
                    <<  "<h1>Current time</h1>\n"
                    <<  "<p>The current time is "
                    <<  my_program_state::now()
                    <<  " seconds since the epoch.</p>\n"
                    <<  "</body>\n"
                    <<  "</html>\n";
        }
        else
        {
            m_response.result(http::status::not_found);
            m_response.set(http::field::content_type, "text/plain");
            beast::ostream(m_response.body()) << "File not found\r\n";
        }
    }

    // Asynchronously transmit the response message.
    void write_response() {
        auto self = shared_from_this();

        m_response.content_length(m_response.body().size());

        http::async_write(
                m_socket,
                m_response,
                [self](beast::error_code ec, std::size_t)
                {
                    self->m_socket.shutdown(tcp::socket::shutdown_send, ec);
                    self->m_deadline.cancel();
                });
    }

    // Check whether we have spent enough time on this connection.
    void check_deadline() {
        auto self = shared_from_this();

        m_deadline.async_wait(
                [self](beast::error_code ec)
                {
                    if(!ec)
                    {
                        // Close socket to cancel any outstanding operation.
                        self->m_socket.close(ec);
                    }
                });
    }
};
*/