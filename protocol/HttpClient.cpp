//
// Created by tate on 7/28/25.
//

#include "FIY.hpp"

#include "HttpClient.hpp"

boost::asio::io_context* HttpClient::get_io_context() {
    return g_fiy->m_ioc;
}