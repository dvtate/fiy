//
// Created by tate on 6/25/25.
//

#pragma once

#include <boost/asio.hpp>

namespace Server {
    void start(boost::asio::io_context* ioc);
}