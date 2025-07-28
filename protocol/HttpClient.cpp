//
// Created by tate on 7/28/25.
//

#include "App.hpp"

#include "HttpClient.hpp"

boost::asio::io_context* HttpClient::get_io_context() {
    return g_app->m_ioc;
}