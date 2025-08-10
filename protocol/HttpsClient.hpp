//
// Created by tate on 7/22/25.
//

#pragma once

#include <boost/asio/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include "globals.hpp"

template <class RequestType, class CallbackType, class ErrCallbackType>
class HttpsRequest : public std::enable_shared_from_this<
                         HttpsRequest<RequestType, CallbackType, ErrCallbackType>> {
    using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

    tcp::resolver m_resolver;
    boost::asio::ssl::stream<boost::beast::tcp_stream> m_stream;
    boost::beast::flat_buffer m_buffer;  // (Must persist between reads)

    std::string m_host;
    std::string m_port;
    RequestType m_req;
    boost::beast::http::response<boost::beast::http::string_body> m_res;
    CallbackType m_cb;
    ErrCallbackType m_err_cb;

public:
    HttpsRequest(
        boost::asio::any_io_executor ex, boost::asio::ssl::context& ssl_ctx, std::string host,
        std::string port, RequestType req, CallbackType cb,
        ErrCallbackType err_cb = [](std::string err) { DEBUG_LOG(err); })
        : m_resolver(ex),
          m_stream(ex, ssl_ctx),
          m_host(std::move(host)),
          m_port(std::move(port)),
          m_req(std::move(req)),
          m_cb(std::move(cb)),
          m_err_cb(std::move(err_cb)) {}

    void run() {
        // Set SNI Hostname (many hosts need this to handshake successfully)
        if (!SSL_set_tlsext_host_name(m_stream.native_handle(), m_host.c_str())) {
            boost::beast::error_code ec{static_cast<int>(::ERR_get_error()),
                                        boost::asio::error::get_ssl_category()};
            m_err_cb(ec.message());
            return;
        }

        // Set the expected hostname in the peer certificate for verification
        m_stream.set_verify_callback(boost::asio::ssl::host_name_verification(m_host.c_str()));

        // Look up the domain name
        m_resolver.async_resolve(
            m_host.c_str(), m_port.c_str(),
            boost::beast::bind_front_handler(&HttpsRequest::on_resolve, this->shared_from_this()));
    }

private:
    void on_resolve(boost::beast::error_code ec, tcp::resolver::results_type results) {
        if (ec) {
            m_err_cb(ec.what());
            return;
        }

        // Set a timeout on the operation
        boost::beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        boost::beast::get_lowest_layer(m_stream).async_connect(
            results,
            boost::beast::bind_front_handler(&HttpsRequest::on_connect, this->shared_from_this()));
    }

    void on_connect(boost::beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
        if (ec) {
            m_err_cb(ec.what());
            return;
        }

        // Perform the SSL handshake
        m_stream.async_handshake(boost::asio::ssl::stream_base::client,
                                 boost::beast::bind_front_handler(&HttpsRequest::on_handshake,
                                                                  this->shared_from_this()));
    }

    void on_handshake(boost::beast::error_code ec) {
        if (ec) {
            m_err_cb(ec.what());
            return;
        }

        // Set a timeout on the operation
        boost::beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(30));

        // Send the HTTP request to the remote host
        boost::beast::http::async_write(
            m_stream, m_req,
            boost::beast::bind_front_handler(&HttpsRequest::on_write, this->shared_from_this()));
    }

    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) {
            m_err_cb(ec.what());
            return;
        }

        // Receive the HTTP response
        boost::beast::http::async_read(
            m_stream, m_buffer, m_res,
            boost::beast::bind_front_handler(&HttpsRequest::on_read, this->shared_from_this()));
    }

    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) {
            m_err_cb(ec.what());
            return;
        }

        // Set a timeout on the operation
        boost::beast::get_lowest_layer(m_stream).expires_after(std::chrono::seconds(10));

        // Gracefully close the stream
        m_stream.async_shutdown(
            boost::beast::bind_front_handler(&HttpsRequest::on_shutdown, this->shared_from_this()));

        // User takes the message and does stuff with it
        m_cb(std::move(m_res));
    }

    void on_shutdown(boost::beast::error_code ec) {
        // ssl::error::stream_truncated, also known as an SSL "short read",
        // indicates the peer closed the connection without performing the
        // required closing handshake (for example, Google does this to
        // improve performance). Generally this can be a security issue,
        // but if your communication protocol is self-terminated (as
        // it is with both HTTP and WebSocket) then you may simply
        // ignore the lack of close_notify.
        //
        // https://github.com/boostorg/beast/issues/38
        //
        // https://security.stackexchange.com/questions/91435/how-to-handle-a-malicious-ssl-tls-shutdown
        //
        // When a short read would cut off the end of an HTTP message,
        // Beast returns the error beast::http::error::partial_message.
        // Therefore, if we see a short read here, it has occurred
        // after the message has been completed, so it is safe to ignore it.

        if (ec != boost::asio::ssl::error::stream_truncated) {
            // Don't call m_err_cb here because we successfully completed the request already
            DEBUG_LOG(ec.what());
            return;
        }
    }
};

class HttpsClient {
    // The SSL context is required, and holds certificates
    boost::asio::ssl::context m_ssl{boost::asio::ssl::context::tlsv12_client};

    void prep_ssl();

    static boost::asio::io_context* get_io_context();

public:
    HttpsClient() {
        prep_ssl();
    }

    template <class RequestType, class CallbackType, class ErrCallbackType>
    void request(std::string host, RequestType req, CallbackType cb, ErrCallbackType err_cb) {
        std::string port = "443";
        const auto colon_pos = host.find(':');
        if (colon_pos != std::string::npos) {
            auto port_str = host.substr(colon_pos + 1);
            if (port_str.size() < 5) {
                auto port_number = atoi(port_str.c_str());
                if (port_number < 65535 && port_number > 0) {
                    port = std::to_string(port_number);
                    host = host.substr(0, colon_pos - 1);
                }
            }
        }

        DEBUG_LOG("FETCH: " << req.method_string() << " https://" << host << ":" << port
                            << req.target());

        std::make_shared<HttpsRequest<RequestType, CallbackType, ErrCallbackType>>(
            boost::asio::make_strand(*get_io_context()), m_ssl, std::move(host), std::move(port),
            std::move(req), std::move(cb), std::move(err_cb))
            ->run();
    }

    template <class RequestType, class CallbackType>
    void request(std::string host, RequestType req, CallbackType cb) {
        request(host, req, cb, [](std::string err) { DEBUG_LOG(err); });
    }
};
