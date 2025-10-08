//
// Created by tate on 7/28/25.
//

#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/strand.hpp>

#include "defs.hpp"

// TODO callback for errors?

template<class RequestType, class CallbackType, class ErrCallbackType>
class HttpRequest : public std::enable_shared_from_this<HttpRequest<RequestType, CallbackType, ErrCallbackType>> {
    using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

    tcp::resolver m_resolver;
    boost::beast::tcp_stream m_stream;
    boost::beast::flat_buffer m_buffer; // (Must persist between reads)

    std::string m_host;
    std::string m_port;
    RequestType m_req;
    boost::beast::http::response<boost::beast::http::string_body> m_res;
    CallbackType m_cb;
    ErrCallbackType m_err_cb;

public:
    HttpRequest(
        boost::asio::io_context* ioc,
        std::string host,
        std::string port,
        RequestType req,
        CallbackType cb,
        ErrCallbackType err_cb = [](std::string err){ (void)err; DEBUG_LOG(err); }
    ):  m_resolver(boost::asio::make_strand(*ioc)),
        m_stream(boost::asio::make_strand(*ioc)),
        m_host(std::move(host)),
        m_port(std::move(port)),
        m_req(std::move(req)),
        m_cb(std::move(cb)),
        m_err_cb(std::move(err_cb))
    {}


    // Start the asynchronous operation
    void run() {
        // Look up the domain name
        m_resolver.async_resolve(
            m_host.c_str(),
            m_port.c_str(),
            boost::beast::bind_front_handler(
                &HttpRequest::on_resolve,
                this->shared_from_this()));
    }

private:
    void on_resolve(boost::beast::error_code ec, tcp::resolver::results_type results) {
        if (ec) {
            m_err_cb(ec.what());
            return;
        }

        // Set a timeout on the operation
        boost::beast::get_lowest_layer(m_stream)
                .expires_after(std::chrono::seconds(30));

        // Make the connection on the IP address we get from a lookup
        m_stream.async_connect(
            results,
            boost::beast::bind_front_handler(
                &HttpRequest::on_connect,
                this->shared_from_this()));
    }

    void on_connect(boost::beast::error_code ec, tcp::resolver::results_type::endpoint_type) {
        if (ec) {
            m_err_cb(ec.what());
            return;
        }

        // Send the request to the remote host
        boost::beast::http::async_write(m_stream, m_req,
            boost::beast::bind_front_handler(
                &HttpRequest::on_write,
                this->shared_from_this()));
    }


    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) {
            m_err_cb(ec.what());
            return;
        }

        // Receive the HTTP response
        boost::beast::http::async_read(m_stream, m_buffer, m_res,
             boost::beast::bind_front_handler(
                 &HttpRequest::on_read,
                 this->shared_from_this()));
    }


    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) {
            m_err_cb(ec.what());
            return;
        }

        // Forward the message
        m_cb(std::move(m_res));

        // Gracefully close the socket
        m_stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes so don't bother reporting it.
        if(ec && ec != boost::beast::errc::not_connected) {
            m_err_cb(ec.what());
            return;
        }

        // If we get here then the connection is closed gracefully
    }

};

class HttpClient {

    static boost::asio::io_context* get_io_context();

public:

    template<class RequestType, class CallbackType, class ErrCallbackType>
    void request(
        std::string host,
        RequestType req,
        CallbackType cb,
        ErrCallbackType err_cb
    ) {
        std::string port = "80";
        const auto colon_pos = host.find(':');
        if (colon_pos != std::string::npos) {
            auto port_str = host.substr(colon_pos + 1);
            if (port_str.size() < 5) {
                auto port_number = atoi(port_str.c_str());
                if (port_number < 65535 && port_number > 0) {
                    port = std::to_string(port_number);
                    host = host.substr(0, colon_pos);
                }
            }
        }

        DEBUG_LOG("FETCH: " <<req.method_string() <<" http://" <<host <<":" <<port <<req.target());

        std::make_shared<HttpRequest<RequestType, CallbackType, ErrCallbackType>>(
            get_io_context(),
            std::move(host),
            std::move(port),
            std::move(req),
            std::move(cb),
            std::move(err_cb)
        )->run();
    }

    template<class RequestType, class CallbackType>
    void request(
        std::string host,
        RequestType req,
        CallbackType cb
    ) {
        request(host, req, cb, [](std::string err){ (void)err; DEBUG_LOG(err); });
    }
};
